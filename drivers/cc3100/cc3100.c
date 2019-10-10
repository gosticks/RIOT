

#include "periph/spi.h"

#include "cc3100.h"
#include "include/cc3100_netdev.h"

#include "vendor/hw_common_reg.h"
#include "vendor/hw_memmap.h"
#include "vendor/rom.h"

#include "include/cc3100_internal.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define ADC_O_ADC_CH_ENABLE 0x000000B8
#define COMMON_REG_BASE 0x400F7000
#define OCP_SHARED_O_SPARE_REG_5 0x0000016C
#define ADC_O_adc_ch7_fifo_lvl \
    0x000000B0 // Channel 7 interrupt status
               // register

/**
 * @brief cc3100_setup resets the network processor (NWP)
 * and sets up an SPI connection far later interactions
 *
 * @param dev
 */
void cc3100_setup(cc3100_t *dev, const cc3100_params_t *params)
{
    dev->nd.driver         = &netdev_driver_cc3100;
    dev->nd.event_callback = NULL;
    dev->nd.context        = dev;
    dev->params            = *params;
    dev->options           = 0;

    printf("HELLO THERE SETUP <3\n");
    /* shutdown nwp subsystem first */

    /* check device spi speed */
    // uint32_t bit_rate = ((GPRCM->GPRCM_DIEID_READ_REG4 >> 24) & 0x02) ?
    //                             SPI_RATE_20M :
    //                             SPI_RATE_30M;

    /* acquire SPI connection to NWP */
    // spi_acquire(1, SPI_CS_UNDEF, SPI_SUB_MODE_0, bit_rate);
}
