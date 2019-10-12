#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>

#include "vendor/rom.h"

#include "net/netdev.h"

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

/* static header buffer */
static cc3100_nwp_resp_header_t _cmd_header = {};

/**
 * @brief cc3100_nwp_rx_handler is the default RX handlers for the NWP
 *
 */
void cc3100_nwp_rx_handler(void)
{
    DEBUG("%s()\n", __FUNCTION__);
    mask_nwp_rx_irqn();

    /* reset header buffer values */
    _cmd_header.GenHeader.Opcode  = 0;
    _cmd_header.GenHeader.Len     = 0;
    _cmd_header.TxPoolCnt         = 0;
    _cmd_header.DevStatus         = 0;
    _cmd_header.SocketTXFailure   = 0;
    _cmd_header.SocketNonBlocking = 0;

    cc31xx_read_cmd_header(_dev, &_cmd_header);
    cc3100_cmd_handler(_dev, &_cmd_header);
}

/**
 * @brief overwrite the default cpu isr handler
 *
 * @param arg
 */
void isr_nwp(void *arg)
{
    netdev_t *dev = (netdev_t *)arg;

    DEBUG("%s()\n", __FUNCTION__);
    // execute default handler
    if (_dev->netdev.netdev.event_callback) {
        // mask_nwp_rx_irqn();
        _dev->netdev.netdev.event_callback(&_dev->netdev.netdev,
                                           NETDEV_EVENT_ISR);
    }
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

    /* store static reference to the dev for isr callback */
    _dev = dev;

    cc3100_nwp_graceful_power_off();
    DEBUG("[cc3100] power off completed\n");
    // TODO: find a way to unify the config at this point

    cc3100_init_nwp(dev);

    /* register handler */
    int16_t err = 0;
    /* delete existing profiles */
    err = _nwp_del_profile(dev, 0xFF);
    if (err != 0) {
        DEBUG("[cc31xx] failed to delete profiles\n");
    }

    if (_nwp_set_wifi_policy(dev, SL_POLICY_SCAN, SL_SCAN_POLICY(0)) != 0) {
        DEBUG("[cc31xx] failed to set scan policy");
    }
    if (_nwp_set_wifi_policy(dev, SL_POLICY_CONNECTION,
                             SL_CONNECTION_POLICY(0, 0, 0, 0, 0)) != 0) {
        DEBUG("[cc31xx] failed to set connect policy");
    }

    if (_nwp_disconnect(dev) != 0) {
        DEBUG("[cc31xx] failed to disconnect \n");
    }

    // disable DHCP
    uint8_t dhcp_disable = 1;
    if (_nwp_set_net_cfg(dev, 4, 0, 1, &dhcp_disable) != 0) {
        DEBUG("[cc31xx] failed to disable DHCP\n");
    }

    // setup wifi power
    // power is a reverse metric (dB)
    uint8_t tx_power = 0;
    if (_nwp_set_wifi_cfg(dev, 1, 10, 1, &tx_power) != 0) {
        DEBUG("[cc31xx] failed to set NWP TX power\n");
    }

    uint8_t filter_cfg[8] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    // reset rx filters
    if (_nwp_set_wifi_filter(dev, 1, filter_cfg,
                             sizeof(_WlanRxFilterOperationCommandBuff_t) !=
                                     0)) {
        DEBUG("[cc31xx]failed to reset wifi filters\n");
    }

    ROM_IntRegister(INT_NWPIC, (void *)isr_nwp);
    NVIC_SetPriority(NWPIC_IRQn, 2);
    NVIC_ClearPendingIRQ(NWPIC_IRQn);
    NVIC_EnableIRQ(NWPIC_IRQn);

    return 0;
}

static void _isr(netdev_t *netdev)
{
    /* notify netdev of finished isr */
    netdev->event_callback(netdev, NETDEV_EVENT_RX_COMPLETE);
}

static int _send(netdev_t *netdev, const iolist_t *iolist)
{
    DEBUG("[CC3100] send\n");
    (void)netdev;
    (void)iolist;
    // cc2420_t *dev = (cc2420_t *)netdev;
    // return (int)cc2420_send(dev, iolist);
    return 0;
}

static int _recv(netdev_t *netdev, void *buf, size_t len, void *info)
{
    DEBUG("[CC3100] recv\n");
    (void)netdev;
    (void)buf;
    (void)len;
    (void)info;
    // cc2420_t *dev = (cc2420_t *)netdev;
    // return (int)cc2420_rx(dev, buf, len, info);
    return 0;
}

static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len)
{
    DEBUG("cc3100_netdev->_get:\n");
    if (netdev == NULL) {
        return -ENODEV;
    }

    cc3100_t *dev = (cc3100_t *)netdev;

    int ext = netdev_ieee80211_get(&dev->netdev, opt, val, max_len);
    if (ext > 0) {
        return ext;
    }

    switch (opt) {
    default:
        DEBUG("UNHANDLED NETOPT :( (%d)\n", opt);
        break;
    }
    (void)netdev;
    (void)opt;
    (void)val;
    (void)max_len;
    return 0;
}

static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t val_len)
{
    if (netdev == NULL) {
        return -ENODEV;
    }

    cc3100_t *dev = (cc3100_t *)netdev;

    DEBUG("[CC3100] set opt=%d \n", opt);
    (void)netdev;
    (void)opt;
    (void)val;
    (void)val_len;
    switch (opt) {
    case NETOPT_RX_START_IRQ:
    case NETOPT_RX_END_IRQ:
        DEBUG("[cc3100] got RX start IRQ\n");
        return 1;
    case NETOPT_TX_START_IRQ:
    case NETOPT_TX_END_IRQ:
        DEBUG("[cc3100] driver does not support advanced interrupt timings\n");
        return 1;
    default:
        return 0;
    }
    return 0;
}
