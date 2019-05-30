/*
 * Copyright (C) 2019 Wlad Meixner
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "driverlib/rom_map.h"
#include "periph/pm.h"


void pm_off(void)
{
    /* No Generic Power off Mechanism */
    // MAP_PRCMMCUReset();
}
