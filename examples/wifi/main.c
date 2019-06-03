/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
// #include "board.h"

#include "periph/cpuid.h"

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
// #include "gpio_if.h"

void blink_test(void) 
{
    // GPIO_IF_LedOff(MCU_ALL_LED_IND);
    while(1)
    {
        //
        // Alternately toggle hi-low each of the GPIOs
        // to switch the corresponding LED on/off.
        //
        // MAP_UtilsDelay(8000000);
        // GPIO_IF_LedOn(MCU_RED_LED_GPIO);
        // MAP_UtilsDelay(8000000);
        // GPIO_IF_LedOff(MCU_RED_LED_GPIO);
        // // MAP_UtilsDelay(8000000);
        // // GPIO_IF_LedOn(MCU_ORANGE_LED_GPIO);
        // // MAP_UtilsDelay(8000000);
        // // GPIO_IF_LedOff(MCU_ORANGE_LED_GPIO);
        // MAP_UtilsDelay(8000000);
        // GPIO_IF_LedOn(MCU_GREEN_LED_GPIO);
        // MAP_UtilsDelay(8000000);
        // GPIO_IF_LedOff(MCU_GREEN_LED_GPIO);
    }
}

static volatile int test = 0;
// static char h[] = "helloThere";

int main(void)
{

    // unsigned char ipin;
    // unsigned int port;
    // ipin = _gpio_pin_to_imux(9);
    // port = _gpio_pin_to_port(9);
    // printf("Pin %c Port %d. \n", ipin, port);
    // GPIO_test_write(9, port, ipin, 0);
    // MAP_UtilsDelay(8000000);
    // GPIO_test_write(9, port, ipin, 1);
    // MAP_UtilsDelay(8000000);
    // GPIO_IF_LedConfigure(LED1|LED2|LED3);

    // GPIO_IF_LedOff(MCU_ALL_LED_IND);
    int i = 0;
    while (i < 10)
    {
        i++;
        test = i + 1;
    }

    uint8_t cpuid[CPUID_LEN];
    cpuid_get(cpuid);
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);

    blink_test();

    return 0;
}
