/*
 * Copyright (C) 2019 Ludwig Maximilian Üniversitaet 
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup        cpu_cc3200_gpio CC3200 General-Purpose I/O
 * @ingroup         cpu_cc3200_regs
 * @{
 *
 * @file
 * @brief           Driver for the cc3200 GPIO controller
 *
 * Header file with register and macro declarations for the cc2538 GPIO module
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 *
 * @{
 */

#ifndef CC3200_PRCM_H
#define CC3200_PRCM_H

#include "cc3200.h"


#ifdef __cplusplus
extern "C" {
#endif

// Copied from TIs driverlib/prcm.h

//*****************************************************************************
//
//  prcm.h
//
//  Prototypes for the PRCM control driver.
//
//  Copyright (C) 2014 Texas Instruments Incorporated - http://www.ti.com/
//
//
//  Redistribution and use in source and binary forms, with or without
//  modification, are permitted provided that the following conditions
//  are met:
//
//    Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
//    Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the
//    distribution.
//
//    Neither the name of Texas Instruments Incorporated nor the names of
//    its contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
//  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
//  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
//  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
//  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
//  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
//  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
//*****************************************************************************

//*****************************************************************************
// Values that can be passed to PRCMPeripheralEnable() and
// PRCMPeripheralDisable()
//*****************************************************************************
#define PRCM_RUN_MODE_CLK         0x00000001
#define PRCM_SLP_MODE_CLK         0x00000100
#define PRCM_DSLP_MODE_CLK        0x00010000

//*****************************************************************************
// Values that can be passed to PRCMSRAMRetentionEnable() and
// PRCMSRAMRetentionDisable() as ulSramColSel.
//*****************************************************************************
#define PRCM_SRAM_COL_1           0x00000001
#define PRCM_SRAM_COL_2           0x00000002
#define PRCM_SRAM_COL_3           0x00000004
#define PRCM_SRAM_COL_4           0x00000008

//*****************************************************************************
// Values that can be passed to PRCMSRAMRetentionEnable() and
// PRCMSRAMRetentionDisable() as ulModeFlags.
//*****************************************************************************
#define PRCM_SRAM_DSLP_RET        0x00000001
#define PRCM_SRAM_LPDS_RET        0x00000002

//*****************************************************************************
// Values that can be passed to PRCMLPDSWakeupSourceEnable(),
// PRCMLPDSWakeupCauseGet() and PRCMLPDSWakeupSourceDisable().
//*****************************************************************************
#define PRCM_LPDS_HOST_IRQ        0x00000080
#define PRCM_LPDS_GPIO            0x00000010
#define PRCM_LPDS_TIMER           0x00000001

//*****************************************************************************
// Values that can be passed to PRCMLPDSWakeUpGPIOSelect() as Type
//*****************************************************************************
#define PRCM_LPDS_LOW_LEVEL       0x00000002
#define PRCM_LPDS_HIGH_LEVEL      0x00000000
#define PRCM_LPDS_FALL_EDGE       0x00000001
#define PRCM_LPDS_RISE_EDGE       0x00000003

//*****************************************************************************
// Values that can be passed to PRCMLPDSWakeUpGPIOSelect()
//*****************************************************************************
#define PRCM_LPDS_GPIO2           0x00000000
#define PRCM_LPDS_GPIO4           0x00000001
#define PRCM_LPDS_GPIO13          0x00000002
#define PRCM_LPDS_GPIO17          0x00000003
#define PRCM_LPDS_GPIO11          0x00000004
#define PRCM_LPDS_GPIO24          0x00000005
#define PRCM_LPDS_GPIO26          0x00000006

//*****************************************************************************
// Values that can be passed to PRCMHibernateWakeupSourceEnable(),
// PRCMHibernateWakeupSourceDisable().
//*****************************************************************************
#define PRCM_HIB_SLOW_CLK_CTR     0x00000001

//*****************************************************************************
// Values that can be passed to PRCMHibernateWakeUpGPIOSelect() as ulType
//*****************************************************************************
#define PRCM_HIB_LOW_LEVEL        0x00000000
#define PRCM_HIB_HIGH_LEVEL       0x00000001
#define PRCM_HIB_FALL_EDGE        0x00000002
#define PRCM_HIB_RISE_EDGE        0x00000003

//*****************************************************************************
// Values that can be passed to PRCMHibernateWakeupSourceEnable(),
// PRCMHibernateWakeupSourceDisable(), PRCMHibernateWakeUpGPIOSelect()
//*****************************************************************************
#define PRCM_HIB_GPIO2            0x00010000
#define PRCM_HIB_GPIO4            0x00020000
#define PRCM_HIB_GPIO13           0x00040000
#define PRCM_HIB_GPIO17           0x00080000
#define PRCM_HIB_GPIO11           0x00100000
#define PRCM_HIB_GPIO24           0x00200000
#define PRCM_HIB_GPIO26           0x00400000

//*****************************************************************************
// Values that will be returned from PRCMSysResetCauseGet().
//*****************************************************************************
#define PRCM_POWER_ON             0x00000000
#define PRCM_LPDS_EXIT            0x00000001
#define PRCM_CORE_RESET           0x00000003
#define PRCM_MCU_RESET            0x00000004
#define PRCM_WDT_RESET            0x00000005
#define PRCM_SOC_RESET            0x00000006
#define PRCM_HIB_EXIT             0x00000007

//*****************************************************************************
// Values that can be passed to PRCMHibernateWakeupCauseGet().
//*****************************************************************************
#define PRCM_HIB_WAKEUP_CAUSE_SLOW_CLOCK  0x00000002
#define PRCM_HIB_WAKEUP_CAUSE_GPIO        0x00000004

//*****************************************************************************
// Values that can be passed to PRCMSEnableInterrupt
//*****************************************************************************
#define PRCM_INT_SLOW_CLK_CTR     0x00004000

//*****************************************************************************
// Values that can be passed to PRCMPeripheralClkEnable(),
// PRCMPeripheralClkDisable(), PRCMPeripheralReset()
//*****************************************************************************
#define PRCM_CAMERA               0x00000000
#define PRCM_I2S                  0x00000001
#define PRCM_SDHOST               0x00000002
#define PRCM_GSPI                 0x00000003
#define PRCM_LSPI                 0x00000004
#define PRCM_UDMA                 0x00000005
#define PRCM_GPIOA0               0x00000006
#define PRCM_GPIOA1               0x00000007
#define PRCM_GPIOA2               0x00000008
#define PRCM_GPIOA3               0x00000009
#define PRCM_GPIOA4               0x0000000A
#define PRCM_WDT                  0x0000000B
#define PRCM_UARTA0               0x0000000C
#define PRCM_UARTA1               0x0000000D
#define PRCM_TIMERA0              0x0000000E
#define PRCM_TIMERA1              0x0000000F
#define PRCM_TIMERA2              0x00000010
#define PRCM_TIMERA3              0x00000011
#define PRCM_DTHE                 0x00000012
#define PRCM_SSPI                 0x00000013
#define PRCM_I2CA0                0x00000014
// Note : PRCM_ADC is a dummy define for pinmux utility code generation
// PRCM_ADC should never be used in any user code.
#define PRCM_ADC                  0x000000FF


#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* CC3200_PRCM_H */

/** @} */
/** @} */