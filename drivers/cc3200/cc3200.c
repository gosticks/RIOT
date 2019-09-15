#define ENABLE_DEBUG (0)
#include "debug.h"

#include "include/cc3200.h"
#include "include/cc3200_params.h"
#include "vendor/hw_common_reg.h"
#include "vendor/hw_memmap.h"

void graceful_nwp_shutdown(void);

/**
 * @brief cc3200_setup resets the network processor (NWP)
 * and sets up an SPI connection far later interactions
 *
 * @param dev
 * @param params
 */
void cc3200_setup(cc3200_t *dev, const cc3200_params_t *params)
{
    /* shutdown nwp subsystem first */
    graceful_nwp_shutdown();

    /* check device spi speed */
    uint32_t bit_rate = ((GPRCM->GPRCM_DIEID_READ_REG4 >> 24) & 0x02) ?
                                SPI_RATE_20M :
                                SPI_RATE_30M;

    /* acquire SPI connection to NWP */
    spi_acquire(1, SPI_CS_UNDEF, SPI_SUB_MODE_0, bit_rate);
}

/**
 * @brief cc3200_init performs NWP initialization leaving the device ready for
 * future commands after this call returns
 *
 * @param dev
 * @return int
 */
int cc3200_init(cc3200_t *dev)
{
    return 0;
}

/**
 * @brief quit all services and power off nwp
 *
 */
// NOTE: probably one of the registers is missaligned right now
void graceful_nwp_shutdown(void)
{
    /* turn of all network services */
    HWREG(COMMON_REG_BASE + ADC_O_ADC_CH_ENABLE) = 1;

    ROM_UtilsDelay(800000 / 5);

    /* check if NWP was powered of or is in some Low Power Deep Sleep state */
    if ((GPRCM->NWP_LPDS_WAKEUP_CFG != 0x20) &&
        !(HWREG(OCP_SHARED_BASE + OCP_SHARED_O_SPARE_REG_5) & 0x2)) {
        uint16_t retry_count = 0;
        /* Loop until APPs->NWP interrupt is cleared or timeout */
        while (retry_count < 1000) {
            /* interrupt gets cleared when NWP has powered on */
            if (!(HWREG(COMMON_REG_BASE + COMMON_REG_O_APPS_INT_STS_RAW) &
                  0x1)) {
                break;
            }
            ROM_UtilsDelay(800000 / 5);
            retry_count++;
        }
    }

    /* Clear APPs to NWP interrupt */
    HWREG(COMMON_REG_BASE + ADC_O_adc_ch7_fifo_lvl) = 1;
    ROM_UtilsDelay(800000 / 5);

    /* power of NWP */
    powerOffWifi();
}