/*
 * Copyright (C) 2019 Wlad Meixner
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

#include "periph/pm.h"
#include "vendor/rom.h"

#ifdef PROVIDES_PM_SET_LOWEST_CORTEXM
// TODO: needs to be impemented
void pm_set_lowest(void) {
  /* this will hibernate with no way to wake up for now */

  // write to the hibernate register
  HWREG(HIB3P3_BASE + HIB3P3_O_MEM_HIB_REQ) = 0x1;

  // wait for 200 uSec
  UtilsDelay((80 * 200) / 3);
}
#endif

void pm_off(void) { /* No Generic Power off Mechanism */
}
