/*
 * Copyright (C) 2019 Ludwig Maximilian Universität 
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     cc3200
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

#include "device.h"


// #define CPU_REV_MASK = 0xF
// #define CPU_PARTNO_MASK = 0xFFF0
// #define CPU_CON_MASK = 0xF0000
// #define CPU_VAR_MASK = 0xF00000
// #define CPU_IMP_MASK = 0xFF000000


void cpuid_get(void *id)
{
    memcpy(id, CPUID_ADDR, CPUID_LEN);
}