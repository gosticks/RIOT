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

#include "cpu.h"

#include <stdbool.h>
#include <stdint.h>

#ifndef PERIPH_CPU_H
#define PERIPH_CPU_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Override mode flank selection values
 *
 * @{
 */
#define HAVE_GPIO_FLANK_T
typedef enum {
  GPIO_FALLING = 0, /**< emit interrupt on falling flank */
  GPIO_BOTH = 1,    /**< emit interrupt on both flanks   */
  GPIO_LOW = 2,     /**< emit interrupt on low level     */
  GPIO_RISING = 4,  /**< emit interrupt on rising flank  */
  GPIO_NONE = 5,    /**< emit interrupt on rising flank  */
  GPIO_HIGH = 6     /**< emit interrupt on low level     */
} gpio_flank_t;

/**
 * @brief   Starting offset of CPU_ID
 */
#define CPUID_ADDR (void *)(0xe000ed00)
/**
 * @brief   Length of the CPU_ID in octets
 */
#define CPUID_LEN (4U)

/**
 * @name    Define a custom type for GPIO pins
 * @{
 */
#define HAVE_GPIO_T
typedef uint32_t gpio_t;
/** @} */

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

#define GPIO_DIR_MODE_IN 0x00000000  // Pin is a GPIO input
#define GPIO_DIR_MODE_OUT 0x00000001 // Pin is a GPIO output

/**
 * @name   UART device configuration
 * @{
 */
typedef struct {
  cc3200_uart_t *dev; /**< pointer to the used UART device */
  gpio_t rx_pin;      /**< pin used for RX */
  gpio_t tx_pin;      /**< pin used for TX */
  gpio_t cts_pin;     /**< CTS pin - set to GPIO_UNDEF when not using */
  gpio_t rts_pin;     /**< RTS pin - set to GPIO_UNDEF when not using */
  //   gpio_t irqn         /**< Interrupt code */
} uart_conf_t;
/** @} */

static const uart_conf_t uart_config[] = {
    {
        .dev = UART0,
        // .freq = CLOCK_CORECLOCK,
        // .pin_rx = GPIO_PIN(PORT_A, 14),
        // .pin_tx = GPIO_PIN(PORT_A, 15),
        // .pcr_rx = PORT_PCR_MUX(3),
        // .pcr_tx = PORT_PCR_MUX(3),
        // .irqn = UART0_RX_TX_IRQn,
        // .scgc_addr = &SIM->SCGC4,
        // .scgc_bit = SIM_SCGC4_UART0_SHIFT,
        // .mode = UART_MODE_8N1,
    },
    {
        .dev = UART1,
        // .freq = CLOCK_CORECLOCK,
        // .pin_rx = GPIO_PIN(PORT_C, 3),
        // .pin_tx = GPIO_PIN(PORT_C, 4),
        // .pcr_rx = PORT_PCR_MUX(3),
        // .pcr_tx = PORT_PCR_MUX(3),
        // .irqn = UART1_RX_TX_IRQn,
        // .scgc_addr = &SIM->SCGC4,
        // .scgc_bit = SIM_SCGC4_UART1_SHIFT,
        // .mode = UART_MODE_8N1,
    },
};
#ifdef __cplusplus
}
#endif

#endif /* PERIPH_CPU_H */
/** @} */
