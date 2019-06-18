/*
 * Copyright (C) 2019 Ludwig Maximilian Universität
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup        cpu_cc3200
 * @{
 *
 * @file
 * @brief           CPU specific definitions and functions for peripheral
 * handling
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 */

#include "cc3200.h"

#include <stdbool.h>
#include <stdint.h>

#ifndef PERIPH_CPU_H
#define PERIPH_CPU_H

#ifdef __cplusplus
extern "C" {
#endif

#define TIMER_NUMOF (4U)

#define T0 TIMER_DEV(0U)
#define T1 TIMER_DEV(1U)
#define T2 TIMER_DEV(2U)
#define T3 TIMER_DEV(3U)

/**
 * @name    Power management configuration
 * @{
 */
#define PROVIDES_PM_SET_LOWEST
#define PROVIDES_PM_RESTART
#define PROVIDES_PM_OFF
/** @} */

// /**
//  * @name   Override the default GPIO mode settings
//  * @{
//  */
// #define HAVE_GPIO_MODE_T
// typedef enum {
//     GPIO_IN         = 0x00000000,     /**< input, no pull */
//     GPIO_IN_PD      = 0x00000100,     /**< input, pull-down */
//     GPIO_IN_PU      = 0x00000200,     /**< input, pull-up */
//     GPIO_IN_ANALOG  = 0x10000000,     /**< input, analog */
//     GPIO_OUT        = 0x00000001,     /**< output */
//     GPIO_OD         = 0x00000010,     /**< open drain */
//     GPIO_OD_PU      = 0x00000110,     /**< open drain pull-up */
//     GPIO_OD_PD      = 0x00000210      /**< open drain pull-down */
// } gpio_mode_t;
// /** @} */

#define CPUID_ADDR (void *)(0xe000ed00)
/**
 * @brief   Length of the CPU_ID in octets
 */
#define CPUID_LEN (4U)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CPU_H */
/** @} */
