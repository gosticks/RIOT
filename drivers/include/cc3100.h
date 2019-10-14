/*
 * Copyright (C) 2015 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @defgroup    drivers_cc2420 CC2420 radio driver
 * @ingroup     drivers_netdev
 * @{
 *
 * @file
 * @brief       Interface definition for the CC2420 driver
 *
 * @author      Thomas Eichinger <thomas.eichinger@fu-berlin.de>
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 */

#ifndef CC3100_H
#define CC3100_H

#include <stdint.h>

#include "periph/gpio.h"
#include "periph/spi.h"

#include "net/netdev.h"
#include "net/netdev/ieee80211.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_CHANNEL (7U)

/**
 * @brief   Struct holding all parameters needed for device initialization
 */
typedef struct cc3200_params {
    spi_t spi;         /**< SPI bus the device is connected to */
    spi_clk_t spi_clk; /**< SPI speed to use */
} cc3100_params_t;

typedef struct {
    netdev_ieee80211_t netdev; /**< extends the netdev structure */
    /* device specific fields */
    cc3100_params_t params; /**< hardware interface configuration */
    /* device state fields */
    uint8_t state;    /**< current state of the radio */
    uint16_t options; /**< state of used options */
    uint16_t sock_id; /**< socket id of the NWP layer two socket */
} cc3100_t;

typedef struct {
    spi_t spi; /**< SPI interface the CC31xx module is connected to */

} cc31xx_conf_t;

/**
 * @brief cc3100_setup resets the network processor (NWP)
 * and sets up an SPI connection for later interactions
 *
 * @param dev
 * @param params
 */
void cc3100_setup(cc3100_t *dev, const cc3100_params_t *params);

/**
 * @brief cc3100 get mac address of the nwp.
 *
 * @param dev
 * @param addr
 */
void cc3100_get_addr(cc3100_t *dev, uint8_t *addr);

#ifdef __cplusplus
}
#endif

#endif /* CC3100_H */
/** @} */
