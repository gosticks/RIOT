/*
 * Copyright (C) 2019 Wlad Meixner
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#ifndef CPU_CONF_H
#define CPU_CONF_H

#include "cpu_conf_common.h"
#include "cc3200.h"
#include "cc3200_spi.h"
#include "cc3200_gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @brief   ARM Cortex-M specific CPU configuration
 * @{
 */
#define CPU_DEFAULT_IRQ_PRIO    (1U) /**< The default priority is 1 for every interrupt, 0 is the highest possible priority. */
#define CPU_IRQ_NUMOF           IRQN_COUNT  
#define CPU_FLASH_BASE          FLASH_BASE /**< number of interrupt sources */
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* CPU_CONF_H */
/** @} */
