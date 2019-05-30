/*
 * Copyright (C) 2019 Ludwig Maximilian Üniversitaet 
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup        cpu_cc3200_gpio CC3200 General-Purpose I/O
 * @ingroup         cpu_cc3200_regs
 * @{
 *
 * @file
 * @brief           Driver for the cc3200 GPIO controller
 *
 * Header file with register and macro declarations for the cc2538 GPIO module
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 *
 * @{
 */

#ifndef CC3200_GPIO_H
#define CC3200_GPIO_H

#include "cc3200.h"


#ifdef __cplusplus
extern "C" {
#endif

/**
 * @name Numeric representation of the four GPIO ports
 * @{
 */
enum {
    GPIO_PORT_A0 = 0,
    GPIO_PORT_A1 = 1,
    GPIO_PORT_A2 = 2,
    GPIO_PORT_A3 = 3,
};


#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* CC3200_GPIO_H */

/** @} */
/** @} */