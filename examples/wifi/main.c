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
#include <string.h>
#define ENABLE_DEBUG (1)
#include "debug.h"
#include "xtimer.h"

// #include "80211.h"
// #include "cmd.h"
// #include "driver.h"
// #include "periph/cpuid.h"
// #include "periph/gpio.h"
// #include "proto.h"
// #include "setup.h"
// #include "state.h"
// #include "utils.h"
#include "vendor/rom.h"

#include "msg.h"
#include "shell.h"

int test(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    puts("WOW");
    return 0;
}

// static const shell_command_t shell_commands[] = { { "test", "test shell",
//                                                     test } };

/* Forward declarations */
int16_t start_pairing(int16_t sd);

int main(void)
{
    // int16_t status = 0;
    printf("2 You are running RIOT on a(n) %s board.\n", RIOT_BOARD);

    /* start shell */
    printf("All up, running the shell now\n");
    // char line_buf[SHELL_DEFAULT_BUFSIZE];
    // shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    return 0;
}
