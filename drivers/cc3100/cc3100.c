

#include "periph/spi.h"

#include "cc3100.h"
#include "include/cc3100_netdev.h"

#include "vendor/hw_common_reg.h"
#include "vendor/hw_memmap.h"
#include "vendor/rom.h"

#include "cc3100_internal.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/**
 * @brief cc3100_setup resets the network processor (NWP)
 * and sets up an SPI connection far later interactions
 *
 * @param dev
 */
void cc3100_setup(cc3100_t *dev, const cc3100_params_t *params)
{
    dev->netdev.netdev.driver = &netdev_driver_cc3100;
    dev->params               = *params;
    dev->options              = 0;
}