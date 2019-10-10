

#include "vendor/rom.h"

#include "net/netdev.h"

#include "cc3100.h"
#include "include/cc3100_internal.h"
#include "include/cc3100_netdev.h"
#include "include/cc3100_registers.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

static int _send(netdev_t *netdev, const iolist_t *iolist);
static int _recv(netdev_t *netdev, void *buf, size_t len, void *info);
static int _init(netdev_t *netdev);
static void _isr(netdev_t *netdev);
static int _get(netdev_t *netdev, netopt_t opt, void *val, size_t max_len);
static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t len);

const netdev_driver_t netdev_driver_cc3100 = {
    .send = _send,
    .recv = _recv,
    .init = _init,
    .isr  = _isr,
    .get  = _get,
    .set  = _set,
};

/**
 * @brief
 *
 * @param arg
 */
static void _isr_handler(void *arg)
{
    DEBUG("[CC3100] isr_handler\n");
    
    netdev_t *dev = (netdev_t *)arg;

    if (dev->event_callback) {
        DEBUG("[CC3100] callback set\n");
        dev->event_callback(dev, NETDEV_EVENT_ISR);
    }
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
    DEBUG("CC3100 init\n");
    cc3100_t *dev = (cc3100_t *)netdev;
    (void)dev;
    cc3100_nwp_graceful_power_off();

    /* register handler */
    ROM_IntRegister(INT_NWPIC, ((cc3100_rx_irqn_handler)_isr_handler));
    ROM_IntPrioritySet(INT_NWPIC, 0x20);
    ROM_IntPendClear(INT_NWPIC);
    ROM_IntEnable(INT_NWPIC);
    DEBUG("CC3100 registered handler\n");

    cc3100_init_nwp();
    return 0;
}

static void _isr(netdev_t *netdev)
{
    DEBUG("CC3100 isr\n");
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

    DEBUG("[CC3100] get\n");
    (void)netdev;
    (void)opt;
    (void)val;
    (void)max_len;
    return 0;
}

static int _set(netdev_t *netdev, netopt_t opt, const void *val, size_t val_len)
{

    DEBUG("[CC3100] set\n");
    (void)netdev;
    (void)opt;
    (void)val;
    (void)val_len;
    return 0;
}

/**
 * @brief cc3100_nwp_rx_handler is the default RX handlers for the NWP
 *
 */
// void cc3100_nwp_rx_handler(void *value)
// {
//     DEBUG("[NWP] handler triggered\n");
//     (void)value;
//     // keep track of the current command count
//     // handledIrqsCount++;
//     mask_nwp_rx_irqn();

//     cc3100_nwp_resp_header_t cmd;
//     cc3100_read_cmd_header(&cmd);
//     printf("HELLO CMD %x \n", cmd.GenHeader.opcode);
//     cc3100_cmd_handler(&cmd);

//     cortexm_isr_end();
// }
