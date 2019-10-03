#ifdef MODULE_CC3200

#include "log.h"
#include "net/gnrc/netif/ieee802154.h"

#include "cc3200.h"

/**
 * @brief   Define stack parameters for the MAC layer thread
 * @{
 */
#define CC3200_MAC_STACKSIZE (THREAD_STACKSIZE_DEFAULT)
// #ifndef CC2538_MAC_PRIO
// #define CC2538_MAC_PRIO (GNRC_NETIF_PRIO)
// #endif

static cc3200_t cc3200_dev;
// static char cc3200_dev_stack[CC3200_MAC_STACKSIZE];

void auto_init_cc3200(void)
{
    LOG_DEBUG("[auto_init_netif] initializing cc3200 radio\n");

    cc3200_setup(&cc3200_dev);
    // gnrc_netif_ieee802154_create(_cc2538_rf_stack, CC2538_MAC_STACKSIZE,
    //                              CC2538_MAC_PRIO, "cc2538_rf",
    //                              (netdev_t *)&cc2538_rf_dev);
}

#else
typedef int dont_be_pedantic;
#endif /* MODULE_CC3200 */
/** @} */
