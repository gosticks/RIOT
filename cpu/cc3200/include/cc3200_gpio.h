/*
 * Copyright (C) 2019 Ludwig Maximilian Universität 
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
 * Header file with register and macro declarations for the cc3200 GPIO module
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 *
 * @{
 */

#ifndef CC3200_GPIO_H
#define CC3200_GPIO_H

#include "cc3200.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**
 * @name Numeric representation of the four GPIO ports
 * @{
 */
    enum
    {
        PORT_A0 = 0,
        PORT_A1 = 1,
        PORT_A2 = 2,
        PORT_A3 = 3,
    };

/**
 * @name    Define a custom type for GPIO pins
 * @{
 */
#define HAVE_GPIO_T
    // typedef uint32_t gpio_t;
/** @} */
/**
 * @brief   Define a CPU specific GPIO pin generator macro
 */
#define GPIO_PIN(x, y) ((x << 6) | (y - 1))
#define GPIO_PINS_PER_PORT 8

#define LED_RED GPIO_PIN(PORT_A1, 64)   //9)
#define LED_ORANGE GPIO_PIN(PORT_A1, 1) //10)
#define LED_GREEN GPIO_PIN(PORT_A1, 2)  //11)
    typedef unsigned long cc3200_gpio_reg;

    typedef struct cc3200_gpio_t
    {
        cc3200_gpio_reg data;           // GPIO Data register
        cc3200_gpio_reg RESERVER1[255]; // GPIO Reserved addresses
        cc3200_gpio_reg dir;            // GPIO Direction register
        cc3200_gpio_reg is;             // GPIO Interrupt Sense
        cc3200_gpio_reg ibe;            // GPIO Interrupt Both Edges
        cc3200_gpio_reg iev;            // GPIO Interrupt Event
        cc3200_gpio_reg im;             // GPIO Interrupt Mask
        cc3200_gpio_reg ris;            // GPIO Raw Interrupt Status
        cc3200_gpio_reg mis;            // GPIO Masked Interrupt Status
        cc3200_gpio_reg icr;            // GPIO Interrupt Clear

    } cc3200_gpio_t;

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* CC3200_GPIO_H */

/** @} */
/** @} */