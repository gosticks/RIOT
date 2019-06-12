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

#include "wifi.h"


int main(void) {
    uint8_t cpuid[CPUID_LEN];
    cpuid_get(cpuid);
    printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
    init_wifi();

    // keept the programm going
    while(1) {
        
    }

    return 0;
}