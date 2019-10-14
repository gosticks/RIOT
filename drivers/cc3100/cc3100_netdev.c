#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "vendor/rom.h"

#include "fmt.h"
#include "net/gnrc.h"
#include "net/netdev.h"
#include "net/netopt.h"

#include "irq_handler.h"
#include "thread.h"
#include "cc3100.h"
#include "include/cc3100_internal.h"
#include "include/cc3100_netdev.h"
#include "include/cc3100_nwp_com.h"
#include "include/cc3100_registers.h"

#include "net/netdev/ieee80211.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static int _send(netdev_t *netdev, const iolist_t *iolist);
static int _recv(netdev_t *netdev, void *buf, size_t len, void *info);
static int _init(netdev_t *netdev);
static void _isr(netdev_t *netdev);
static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len);
static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len);

static irq_event_t _irq_nwp_event = IRQ_EVENT_INIT;


extern uint8_t _cc31xx_isr_state;

void _irq_event(void *args) {

    (void)args;
    mask_nwp_rx_irqn();
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
    DEBUG("%s()\n", __FUNCTION__);
    // cc3100_t *dev = (cc3100_t *)arg;

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

    cc3100_nwp_graceful_power_off();
    DEBUG("[cc3100] power off completed\n");

    /* INIT NWP */
    cc3100_init_nwp(dev);

    /* setup the nwp */
    _nwp_setup(dev);

    ROM_IntRegister(INT_NWPIC, (void *)isr_nwp);
    NVIC_SetPriority(NWPIC_IRQn, 2);
    NVIC_ClearPendingIRQ(NWPIC_IRQn);
    NVIC_EnableIRQ(NWPIC_IRQn);
    return 0;
}

static void _isr(netdev_t *netdev)
{
    puts("###ISR");
    /* notify netdev of finished isr */
    netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
}

static int _send(netdev_t *netdev, const iolist_t *iolist)
{
    cc3100_t *dev = (cc3100_t *)netdev;
        iolist->
    int pkt_len = 0;
    /* send items in a packet one after the other */
    // TODO: pack multiple packets into one packet
    for (const iolist_t *iol = iolist; iol; iol = iol->iol_next) {
        
        pkt_len += iol->iol_len;
        _nwp_send_raw_frame(dev, dev->sock_id, iol->iol_base, iol->iol_len, SL_RAW_RF_TX_PARAMS(WIFI_CHANNEL, 1, 0, 0));
    }
    return 0;
}

static int _recv(netdev_t *netdev, void *buf, size_t len, void *info)
{
    cc3100_t *dev = (cc3100_t *)netdev;
    int16_t size = _nwp_read_raw_frame(dev, dev->sock_id, buf, len, 0);
    return size;
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
        return 1;
    case NETOPT_RX_START_IRQ:
    case NETOPT_TX_START_IRQ:
    case NETOPT_TX_END_IRQ:
        return 1;
    default:
        return 0;
    }
    return -ENODEV;
}
