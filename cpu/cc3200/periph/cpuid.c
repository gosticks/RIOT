/*
 * Copyright (C) 2019 Ludwig Maximilian Universität
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cpu_cc3200
 * @{
 *
 * @file
 * @brief       CPUID driver implementation
 *
 * @author      Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 *
 * @}
 */

#include <string.h>

#include "cpu.h"
#include "periph/cpuid.h"

typedef struct {
    unsigned char rev : 4;
    unsigned char part_no_2 : 8;
    unsigned char part_no_1 : 4;
    unsigned char con : 4;
    unsigned char var : 4;
    unsigned char imp : 8;
} cpuid_t;

void cpuid_get(void *id)
{
    cpuid_t *cpuid     = ((cpuid_t *)CPUID_ADDR);
    unsigned char *_id = id;
    _id[0]             = cpuid->rev;
    _id[1]             = cpuid->part_no_1;
    _id[2]             = cpuid->part_no_2;
    _id[3]             = cpuid->con;
    _id[4]             = cpuid->var;
    _id[5]             = cpuid->imp;
}