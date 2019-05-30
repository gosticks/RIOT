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

#include "cpu.h"
#include "board.h"
#include "vendor/hw_types.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_gpio.h"
#include "periph/gpio.h"
#include "driverlib/pin.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/gpio.h"
#include "driverlib/utils.h"
#include "driverlib/prcm.h"


// #include "periph/gpio.h"
extern const void cortex_vector_base;

/**
 * @brief Initialize on-board LEDs
 */
void led_init(void)
{
     //
    // Enable Peripheral Clocks 
    //
    MAP_PRCMPeripheralClkEnable(PRCM_GPIOA1, PRCM_RUN_MODE_CLK);

    //
    // Configure PIN_64 for GPIOOutput
    //
    MAP_PinTypeGPIO(PIN_64, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA1_BASE, 0x2, GPIO_DIR_MODE_OUT);

    //
    // Configure PIN_01 for GPIOOutput
    //
    MAP_PinTypeGPIO(PIN_01, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA1_BASE, 0x4, GPIO_DIR_MODE_OUT);

    //
    // Configure PIN_02 for GPIOOutput
    //
    MAP_PinTypeGPIO(PIN_02, PIN_MODE_0, false);
    MAP_GPIODirModeSet(GPIOA1_BASE, 0x8, GPIO_DIR_MODE_OUT);
}


/**
 * @brief Initialize the board
 */
void board_init(void)
{
    /* initialize the CPU */
    cpu_init();

    /* initialize the boards LEDs */
    led_init();
}

/** @} */
