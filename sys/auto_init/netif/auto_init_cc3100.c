#ifdef MODULE_CC3100

#include "log.h"
#include "net/gnrc/netif/ieee80211.h"
#include "net/gnrc.h"

#include "cc3100.h"
#include "cc3100_params.h"

/**
 * @brief   Define stack parameters for the MAC layer thread
 * @{
 */
#define CC3100_MAC_STACKSIZE (THREAD_STACKSIZE_DEFAULT)
#ifndef CC3100_MAC_PRIO
#define CC3100_MAC_PRIO (GNRC_NETIF_PRIO)
#endif

static cc3100_t cc3100_dev;
static char cc3100_dev_stack[CC3100_MAC_STACKSIZE];

void auto_init_cc3100(void)
{
    LOG_DEBUG("[auto_init_netif] initializing cc3100 radio\n");

    cc3100_setup(&cc3100_dev, &cc3100_params);
    gnrc_netif_ieee80211_create(cc3100_dev_stack, CC3100_MAC_STACKSIZE,
                                 CC3100_MAC_PRIO, "cc3100",
                                 (netdev_t *)&cc3100_dev);
}

#else
typedef int dont_be_pedantic;
#endif /* MODULE_CC3100 */
/** @} */
