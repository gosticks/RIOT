#ifndef CC3100_PARAMS_H
#define CC3100_PARAMS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "board.h"

#ifndef CC3100_PARAM_SPI
#define CC3100_PARAM_SPI (SPI_DEV(1))
#endif
#ifndef CC3100_PARAM_SPI_CLK
#define CC3100_PARAM_SPI_CLK (SPI_CLK_20MHZ)
#endif
#ifndef CC3100_PARAMS
#define CC3100_PARAMS                                             \
    {                                                             \
        .spi = CC3100_PARAM_SPI, .spi_clk = CC3100_PARAM_SPI_CLK, \
    }
#endif
/**@}*/

/**
 * @brief   CC2420 configuration
 */
static const cc3100_params_t cc3100_params = CC3100_PARAMS;

#ifdef __cplusplus
}
#endif

#endif /* CC3100_PARAMS_H */
       /** @} */