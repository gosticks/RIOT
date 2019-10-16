#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#ifdef CPU_CC3200
#include "vendor/rom.h"
#endif

#include "fmt.h"
#include "irq_handler.h"
#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/netdev.h"
#include "net/netopt.h"
#include "thread.h"
#include "xtimer.h"

#include "cc3100.h"
#include "include/cc3100_internal.h"
#include "include/cc3100_netdev.h"
#include "include/cc3100_nwp_com.h"
#include "include/cc3100_registers.h"
#include "periph/spi.h"

#include "net/netdev/ieee80211.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#if defined(MODULE_OD) && ENABLE_DEBUG
#include "od.h"
#endif

char RawData_Ping[] = {
    /*---- wlan header start -----*/
    0x88,                               /* version , type sub type */
    0x02,                               /* Frame control flag */
    0x2C, 0x00,                         /* Duration ID */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, /* destination */
    0x22, 0x33, 0x44, 0x55, 0x66, 0x77, /* bssid */
    0xe0, 0xe5, 0xcf, 0xbc, 0x71, 0xd0, /* source */
    // 0x80, 0x42, 0x00, 0x00, 0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08,
    // 0x00, /* LLC */
    // /*---- ip header start -----*/
    // 0x08, 0x00,
    0x63, 0x63, 0x33, 0x31, 0x78, 0x78, 0x20, 0x64, 0x61, 0x74, 0x61, 0x20,
    0x74, 0x65, 0x73, 0x74, 0x20, 0x74, 0x72, 0x61, 0x6e, 0x73, 0x6d, 0x69,
    0x73, 0x73, 0x69, 0x6f, 0x6e
    // 0x54, 0x65, 0x73, 0x74, 0x20, 0x74, 0x72, 0x61, 0x6e, 0x73, 0x6d, 0x69,
    // 0x73, 0x73, 0x69, 0x6f, 0x6e
    // 0x45, 0x00, 0x00, 0x54, 0x96, 0xA1, 0x00, 0x00, 0x40, 0x01, 0x57,
    // 0xFA,                   /* checksum */
    // 0xff, 0xff, 0xff, 0xff, /* src ip */
    // 0xc0, 0xa8, 0x01, 0x02, /* dest ip  */
    /* payload - ping/icmp */
    // 0x08, 0x00, 0xA5, 0x51, 0x5E, 0x18, 0x00, 0x00, 0x41, 0x08, 0xBB, 0x8D,
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    // 0x00, 0x00, 0x00, 0x00, .....
};

static int _send(netdev_t *netdev, const iolist_t *iolist);
static int _recv(netdev_t *netdev, void *buf, size_t len, void *info);
static int _init(netdev_t *netdev);
static void _isr(netdev_t *netdev);
static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len);
static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len);

static irq_event_t _irq_nwp_event = IRQ_EVENT_INIT;

extern uint8_t _cc31xx_isr_state;

void _irq_event(void *args)
{
    (void)args;
    _irq_nwp_event.isr(_irq_nwp_event.ctx);
    cortexm_isr_end();
}

/**
 * @brief overwrite the default cpu isr handler
 *
 * @param arg
 */
void isr_nwp(void *arg)
{
    // DEBUG("%s()\n", __FUNCTION__);
    // cc3100_t *dev = (cc3100_t *)arg;

    mask_nwp_rx_irqn();
    // execute default handler
    if (_dev->netdev.netdev.event_callback) {
        _dev->netdev.netdev.event_callback(&_dev->netdev.netdev,
                                           NETDEV_EVENT_ISR);
    }
    _cc31xx_isr_state = 1;
}

const netdev_driver_t netdev_driver_cc3100 = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr  = _isr,
    .get  = _get,
    .set  = _set,
};

static inline int opt_state(void *buf, bool cond)
{
    *((netopt_enable_t *)buf) = !!(cond);
    return sizeof(netopt_enable_t);
}

char rcv_thread_stack[THREAD_STACKSIZE_MAIN];
void *rcv_thread(void *arg)
{
    while (true) {
        DEBUG("[cc31xx] received frame called");
        // _nwp_send_raw_frame(_dev, _dev->sock_id, &RawData_Ping,
        //                     sizeof(RawData_Ping),
        //                     SL_RAW_RF_TX_PARAMS(WIFI_CHANNEL, 1, 0, 0));
        _nwp_req_rcv_frame(_dev, _dev->sock_id, 0);
        thread_yield();
    }

    // _nwp_req_rcv_frame
}

/**
 * @brief cc3100_init performs NWP initialization leaving the device ready for
 * future commands after this call returns
 *
 * @param netdev
 * @return int
 */
static int _init(netdev_t *netdev)
{
    DEBUG("[cc3100] init\n");
    cc3100_t *dev = (cc3100_t *)netdev;
    DEBUG("[cc3100] init %d\n", dev->params.spi);

    /* if we are on a CC32xx platform the NWP has its own interrupt
       so create and register a RIOT irq_event
     */
    _irq_nwp_event.isr = isr_nwp;
    _irq_nwp_event.ctx = &dev;

    /* store static reference to the dev for isr callback */
    _dev = dev;

    /* acquire spi connection */
    spi_acquire(_dev->params.spi, SPI_CS_UNDEF, SPI_SUB_MODE_0,
                _dev->params.spi_clk);

    cc3100_nwp_graceful_power_off();
    DEBUG("[cc3100] power off completed\n");

    /* INIT NWP */
    cc3100_init_nwp(dev);

    /* setup the nwp */
    _nwp_setup(dev);

    // while (true) {
    //     _nwp_send_raw_fragments(_dev, _dev->sock_id, sizeof(RawData_Ping),
    //                             SL_RAW_RF_TX_PARAMS(WIFI_CHANNEL, 1, 0, 0));
    //     DEBUG("Transmit fragment\n");
    //     cc31xx_send_to_nwp(dev, RawData_Ping, sizeof(RawData_Ping) - 8);
    //     // DEBUG("Transmit fragment\n");
    //     cc31xx_send_to_nwp(dev, &RawData_Ping[sizeof(RawData_Ping) - 9], 8);
    // }
    // while (true) {
    //     _nwp_send_raw_frame(_dev, _dev->sock_id, RawData_Ping,
    //                         sizeof(RawData_Ping),
    //                         SL_RAW_RF_TX_PARAMS(WIFI_CHANNEL, 1, 0, 0));
    // }

    // _nwp_set_mac_filter(dev, dev->netdev.addr);
    //     while (true) {
    //         DEBUG("[cc31xx]: reading frames \n");
    //         size_t len = _nwp_read_raw_frame(_dev, _dev->sock_id,
    //         sharedBuffer,
    //                                          sizeof(sharedBuffer), 0);

    // #if ENABLE_DEBUG
    //         DEBUG("[cc31xx]: transceiver mode \n");
    // #if defined(MODULE_OD)
    //         od_hex_dump(sharedBuffer, sizeof(sharedBuffer),
    //         OD_WIDTH_DEFAULT);
    // #endif
    // #endif
    //     }
    /* configure filter for the current device */

#ifdef CPU_CC3200
    ROM_IntRegister(INT_NWPIC, (void *)isr_nwp);
    NVIC_SetPriority(NWPIC_IRQn, 2);
    NVIC_ClearPendingIRQ(NWPIC_IRQn);
    NVIC_EnableIRQ(NWPIC_IRQn);
#else
    DEBUG("NON CC3200 board requires custom setup");
#endif

    /* trigger receive */
    // rcv_thread(NULL);

    /* create receive thread */
    thread_create(rcv_thread_stack, sizeof(rcv_thread_stack),
                  THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, rcv_thread,
                  NULL, "rcv_thread");

    return 0;
}

static void _isr(netdev_t *netdev)
{
    puts("ISR called");
    cc31xx_read_cmd_header(_dev, &_cmd_header);
    cc3100_cmd_handler(_dev, &_cmd_header);
    /* notify netdev of finished isr */
    netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
}

static int _send(netdev_t *netdev, const iolist_t *iolist)
{
    cc3100_t *dev  = (cc3100_t *)netdev;
    size_t pkt_len = 0;
    /* send items in a packet one after the other */
    // TODO: pack multiple packets into one packet
    for (const iolist_t *iol = iolist; iol; iol = iol->iol_next) {
        pkt_len += iol->iol_len;
    }

    /* transmit raw frame header with total length */
    _nwp_send_raw_frame(dev, dev->sock_id, NULL, pkt_len,
                        SL_RAW_RF_TX_PARAMS(WIFI_CHANNEL, 1, 0, 0));

    /* transmit payload */
    // spi_acquire(dev->params.spi, SPI_CS_UNDEF, SPI_SUB_MODE_0,
    //             dev->params.spi_clk);
    for (const iolist_t *iol = iolist; iol; iol = iol->iol_next) {
#if ENABLE_DEBUG
        DEBUG("[cc31xx]: transmitting %d byte \n", iol->iol_len);
#if defined(MODULE_OD)
        od_hex_dump(iol->iol_base, iol->iol_len, OD_WIDTH_DEFAULT);
#endif
#endif
        cc31xx_send_to_nwp(dev, iol->iol_base, iol->iol_len);
    }
    // spi_release(dev->params.spi);
    // rcv_thread(NULL);
    DEBUG("[cc31xx] transmission completed \n");
    return 0;
}

static int _recv(netdev_t *netdev, void *buf, size_t len, void *info)
{
    cc3100_t *dev = (cc3100_t *)netdev;

    /* failed driver read case */
    if (_cmd_header.GenHeader.Opcode == 0) {
        /* request a new frame */
        _nwp_req_rcv_frame(dev, dev->sock_id, 0);
        return 0;
    }

    /* check if data command came from NWP */
    if (_cmd_header.GenHeader.Opcode == 0x100a) {
        size_t size =
                ((_cmd_header.GenHeader.Len - sizeof(_ti_header)) + 3) & (~3);
        /* netdev interface may call _recv to estimate package
           size */
        if (len == 0) {
            if (size == 0) {
                // spi_acquire(dev->params.spi, SPI_CS_UNDEF, SPI_SUB_MODE_0,
                // dev->params.spi_clk);
                /* read TI proprietary header */
                cc3100_read_from_nwp(dev, &_ti_header, sizeof(_ti_header));

                // spi_release(dev->params.spi);
                // rcv_thread(NULL);
            }

            return size;
        }

        /* read socket info */
        _SocketResponse_t sock_resp = { 0 };
        cc3100_read_from_nwp(dev, &sock_resp, sizeof(_SocketResponse_t));

        DEBUG("[cc31xx] read frame body status/len %d \n",
              sock_resp.statusOrLen);

        int16_t remainder = sock_resp.statusOrLen;

        /* read TI proprietary header */
        cc3100_read_from_nwp(dev, &_ti_header, sizeof(_ti_header));

        /* set packet status information */
        // ((netdev_radio_rx_info *)info)->rssi = _ti_header.rssi;

        if (buf == NULL) {
            // spi_release(dev->params.spi);
            // rcv_thread(NULL);
            return 0;
        }

        /* payload can sometimes be smaller then expected */
        if (remainder < (int16_t)len) {
            cc3100_read_from_nwp(dev, buf, remainder);
            remainder = 0;
        } else {
            remainder -= len;
            // DEBUG("NWP: Read payload %d bytes \n", len);
            if (len > 0 && remainder >= 0) {
                cc3100_read_from_nwp(dev, buf, len);
            }
        }
        /* make sure the connection is read to the end */
        if (len % 4 > 0) {
            cc3100_read_from_nwp(dev, sharedBuffer, 4 - (len % 4));
        }

        // /* read all remaining data */
        // if (remainder > 0) {
        //     cc3100_read_from_nwp(dev, sharedBuffer, remainder);
        // }
        // // rcv_thread(NULL);
#if ENABLE_DEBUG
        DEBUG("[cc31xx] recv frame rssi=(%d), speed=(%d)\n", _ti_header.rssi,
              _ti_header.rate);
#if defined(MODULE_OD)
        od_hex_dump(buf, len, OD_WIDTH_DEFAULT);
#endif
#endif

        // spi_release(dev->params.spi);
        return _cmd_header.GenHeader.Len;
    }
    return 0;
}

static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len)
{
    DEBUG("%s(%s)\n", __FUNCTION__, netopt2str(opt));
    if (netdev == NULL) {
        return -ENODEV;
    }

    cc3100_t *dev = (cc3100_t *)netdev;

    int ext = netdev_ieee80211_get(&dev->netdev, opt, val, max_len);
    if (ext > 0) {
        return ext;
    }

    // switch (opt) {
    // default:
    //     DEBUG("UNHANDLED NETOPT :( (%d)\n", opt);
    //     break;
    // }
    return -ENODEV;
}

static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t val_len)
{
    DEBUG("%s(%s)\n", __FUNCTION__, netopt2str(opt));
    if (netdev == NULL) {
        return -ENODEV;
    }

    cc3100_t *dev = (cc3100_t *)netdev;

    int ext = netdev_ieee80211_set(&dev->netdev, opt, val, val_len);
    if (ext > 0) {
        return ext;
    }
    switch (opt) {
    case NETOPT_RX_END_IRQ:
    case NETOPT_RX_START_IRQ:
    case NETOPT_TX_START_IRQ:
    case NETOPT_TX_END_IRQ:
        return 1;
    default:
        return 0;
    }
    return -ENODEV;
}
