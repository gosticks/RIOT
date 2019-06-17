/*
 * Copyright (C) 2015 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     boards_cc3200
 * @{
 *
 * @file
 * @brief       Board specific implementations for the CC3200 launchpad board
 *
 * @author      Attilio Dona'
 */
// #include "vendor/hw_ints.h"
// #include "vendor/hw_memmap.h"
// #include "vendor/hw_types.h"
// #include <stdio.h>

// #include "driverlib/interrupt.h"
// #include "driverlib/utils.h"

#include "board.h"
#include "cpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin.h"
#include "driverlib/prcm.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/utils.h"
#include "periph/gpio.h"
#include "vendor/hw_gpio.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_types.h"

// #include "periph/gpio.h"
extern const void cortex_vector_base;

/**
 * @brief Initialize on-board LEDs
 */
void led_init(void) {
  //
  // Enable Peripheral Clocks
  //
  MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);

  // enable leads and clear them
  gpio_init(LED_RED, GPIO_OUT);
  gpio_clear(LED_RED);

  gpio_init(LED_ORANGE, GPIO_OUT);
  gpio_clear(LED_ORANGE);

  gpio_init(LED_GREEN, GPIO_OUT);
  gpio_clear(LED_GREEN);
}

/**
 * @brief Initialize the board
 */
void board_init(void) {
  /* initialize the CPU */
  cpu_init();

  /* initialize the boards LEDs */
  led_init();
}

/** @} */
