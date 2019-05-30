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
 * @brief           CPU specific definitions and functions for peripheral handling
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 */

#include "cc3200.h"

#ifndef PERIPH_CPU_H
#define PERIPH_CPU_H

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @name    Power management configuration
 * @{
 */
// #define PROVIDES_PM_SET_LOWEST
// #define PROVIDES_PM_RESTART
#define PROVIDES_PM_OFF
/** @} */


#define CPUID_ADDR (*(cc3200_reg_t*)0xE000ED00)

/**
 * @brief   Length of the CPU_ID in octets
 */
#define CPUID_LEN           (4U)
/** @} */

#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CPU_H */
/** @} */
