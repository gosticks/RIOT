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
extern "C"
{
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
#define PRCM_RUN_MODE_CLK 0x00000001
#define PRCM_SLP_MODE_CLK 0x00000100
#define PRCM_DSLP_MODE_CLK 0x00010000

//*****************************************************************************
// Values that can be passed to PRCMSRAMRetentionEnable() and
// PRCMSRAMRetentionDisable() as ulSramColSel.
//*****************************************************************************
#define PRCM_SRAM_COL_1 0x00000001
#define PRCM_SRAM_COL_2 0x00000002
#define PRCM_SRAM_COL_3 0x00000004
#define PRCM_SRAM_COL_4 0x00000008

//*****************************************************************************
// Values that can be passed to PRCMSRAMRetentionEnable() and
// PRCMSRAMRetentionDisable() as ulModeFlags.
//*****************************************************************************
#define PRCM_SRAM_DSLP_RET 0x00000001
#define PRCM_SRAM_LPDS_RET 0x00000002

//*****************************************************************************
// Values that can be passed to PRCMLPDSWakeupSourceEnable(),
// PRCMLPDSWakeupCauseGet() and PRCMLPDSWakeupSourceDisable().
//*****************************************************************************
#define PRCM_LPDS_HOST_IRQ 0x00000080
#define PRCM_LPDS_GPIO 0x00000010
#define PRCM_LPDS_TIMER 0x00000001

//*****************************************************************************
// Values that can be passed to PRCMLPDSWakeUpGPIOSelect() as Type
//*****************************************************************************
#define PRCM_LPDS_LOW_LEVEL 0x00000002
#define PRCM_LPDS_HIGH_LEVEL 0x00000000
#define PRCM_LPDS_FALL_EDGE 0x00000001
#define PRCM_LPDS_RISE_EDGE 0x00000003

//*****************************************************************************
// Values that can be passed to PRCMLPDSWakeUpGPIOSelect()
//*****************************************************************************
#define PRCM_LPDS_GPIO2 0x00000000
#define PRCM_LPDS_GPIO4 0x00000001
#define PRCM_LPDS_GPIO13 0x00000002
#define PRCM_LPDS_GPIO17 0x00000003
#define PRCM_LPDS_GPIO11 0x00000004
#define PRCM_LPDS_GPIO24 0x00000005
#define PRCM_LPDS_GPIO26 0x00000006

//*****************************************************************************
// Values that can be passed to PRCMHibernateWakeupSourceEnable(),
// PRCMHibernateWakeupSourceDisable().
//*****************************************************************************
#define PRCM_HIB_SLOW_CLK_CTR 0x00000001

//*****************************************************************************
// Values that can be passed to PRCMHibernateWakeUpGPIOSelect() as ulType
//*****************************************************************************
#define PRCM_HIB_LOW_LEVEL 0x00000000
#define PRCM_HIB_HIGH_LEVEL 0x00000001
#define PRCM_HIB_FALL_EDGE 0x00000002
#define PRCM_HIB_RISE_EDGE 0x00000003

//*****************************************************************************
// Values that can be passed to PRCMHibernateWakeupSourceEnable(),
// PRCMHibernateWakeupSourceDisable(), PRCMHibernateWakeUpGPIOSelect()
//*****************************************************************************
#define PRCM_HIB_GPIO2 0x00010000
#define PRCM_HIB_GPIO4 0x00020000
#define PRCM_HIB_GPIO13 0x00040000
#define PRCM_HIB_GPIO17 0x00080000
#define PRCM_HIB_GPIO11 0x00100000
#define PRCM_HIB_GPIO24 0x00200000
#define PRCM_HIB_GPIO26 0x00400000

//*****************************************************************************
// Values that will be returned from PRCMSysResetCauseGet().
//*****************************************************************************
#define PRCM_POWER_ON 0x00000000
#define PRCM_LPDS_EXIT 0x00000001
#define PRCM_CORE_RESET 0x00000003
#define PRCM_MCU_RESET 0x00000004
#define PRCM_WDT_RESET 0x00000005
#define PRCM_SOC_RESET 0x00000006
#define PRCM_HIB_EXIT 0x00000007

//*****************************************************************************
// Values that can be passed to PRCMHibernateWakeupCauseGet().
//*****************************************************************************
#define PRCM_HIB_WAKEUP_CAUSE_SLOW_CLOCK 0x00000002
#define PRCM_HIB_WAKEUP_CAUSE_GPIO 0x00000004

//*****************************************************************************
// Values that can be passed to PRCMSEnableInterrupt
//*****************************************************************************
#define PRCM_INT_SLOW_CLK_CTR 0x00004000

//*****************************************************************************
// Values that can be passed to PRCMPeripheralClkEnable(),
// PRCMPeripheralClkDisable(), PRCMPeripheralReset()
//*****************************************************************************
#define PRCM_CAMERA 0x00000000
#define PRCM_I2S 0x00000001
#define PRCM_SDHOST 0x00000002
#define PRCM_GSPI 0x00000003
#define PRCM_LSPI 0x00000004
#define PRCM_UDMA 0x00000005
#define PRCM_GPIOA0 0x00000006
#define PRCM_GPIOA1 0x00000007
#define PRCM_GPIOA2 0x00000008
#define PRCM_GPIOA3 0x00000009
#define PRCM_GPIOA4 0x0000000A
#define PRCM_WDT 0x0000000B
#define PRCM_UARTA0 0x0000000C
#define PRCM_UARTA1 0x0000000D
#define PRCM_TIMERA0 0x0000000E
#define PRCM_TIMERA1 0x0000000F
#define PRCM_TIMERA2 0x00000010
#define PRCM_TIMERA3 0x00000011
#define PRCM_DTHE 0x00000012
#define PRCM_SSPI 0x00000013
#define PRCM_I2CA0 0x00000014
// Note : PRCM_ADC is a dummy define for pinmux utility code generation
// PRCM_ADC should never be used in any user code.
#define PRCM_ADC 0x000000FF

#define PRCM_SOFT_RESET 0x00000001
#define PRCM_ENABLE_STATUS 0x00000002

    typedef struct cc3200_periph_regs_t
    {
        cc3200_reg_t clk_gating;
        cc3200_reg_t soft_reset;
    } cc3200_periph_regs_t;

    typedef struct cc3200_arcm_t
    {
        cc3200_reg_t CAM_CLK_GEN;
        cc3200_periph_regs_t CAM;
        cc3200_reg_t RESERVED1[2];
        cc3200_periph_regs_t MCASP;
        cc3200_reg_t RESERVED2[1];
        cc3200_reg_t MMCHS_CLK_GEN;
        cc3200_periph_regs_t MMCHS;
        cc3200_reg_t MCSPI_A1_CLK_GEN;
        cc3200_periph_regs_t MCSPI_A1;
        cc3200_reg_t MCSPI_A2_CLK_GEN;
        cc3200_reg_t RESERVED3[1];
        cc3200_periph_regs_t MCSPI_A2;
        cc3200_periph_regs_t UDMA_A;
        cc3200_periph_regs_t GPIO_A;
        cc3200_periph_regs_t GPIO_B;
        cc3200_periph_regs_t GPIO_C;
        cc3200_periph_regs_t GPIO_D;
        cc3200_periph_regs_t GPIO_E;
        cc3200_periph_regs_t WDOG_A;
        cc3200_periph_regs_t UART_A0;
        cc3200_periph_regs_t UART_A1;
        cc3200_periph_regs_t GPT_A0;
        cc3200_periph_regs_t GPT_A1;
        cc3200_periph_regs_t GPT_A2;
        cc3200_periph_regs_t GPT_A3;
        cc3200_reg_t MCASP_FRAC_CLK_CONFIG0;
        cc3200_reg_t MCASP_FRAC_CLK_CONFIG1;
        cc3200_periph_regs_t CRYPTO;
        cc3200_reg_t RESERVED4[2];
        cc3200_periph_regs_t MCSPI_S0;
        cc3200_reg_t MCSPI_S0_CLKDIV_CFG;
        cc3200_reg_t RESERVED5[1];
        cc3200_periph_regs_t I2C;
    } cc3200_arcm_t;

#define ARCM ((cc3200_arcm_t *)0x44025000) /**< One and only instance of the System Control module */

    typedef struct cc3200_gprcm_t
    {
        cc3200_reg_t APPS_SOFT_RESET;
        cc3200_reg_t APPS_LPDS_WAKEUP_CFG;
        cc3200_reg_t APPS_LPDS_WAKEUP_SRC;
        cc3200_reg_t APPS_RESET_CAUSE;
        cc3200_reg_t APPS_LPDS_WAKETIME_OPP_CFG;
        cc3200_reg_t RESERVER1[1];
        cc3200_reg_t APPS_SRAM_DSLP_CFG;
        cc3200_reg_t APPS_SRAM_LPDS_CFG;
        cc3200_reg_t APPS_LPDS_WAKETIME_WAKE_CFG;
        cc3200_reg_t RESERVER2[55];
        cc3200_reg_t TOP_DIE_ENABLE;
        cc3200_reg_t TOP_DIE_ENABLE_PARAMETERS;
        cc3200_reg_t MCU_GLOBAL_SOFT_RESET;
        cc3200_reg_t ADC_CLK_CONFIG;
        cc3200_reg_t APPS_GPIO_WAKE_CONF;
        cc3200_reg_t EN_NWP_BOOT_WO_DEVINIT;
        cc3200_reg_t MEM_HCLK_DIV_CFG;
        cc3200_reg_t MEM_SYSCLK_DIV_CFG;
        cc3200_reg_t APLLMCS_LOCK_TIME_CONF;
        cc3200_reg_t RESERVER3[183];
        cc3200_reg_t NWP_SOFT_RESET;
        cc3200_reg_t NWP_LPDS_WAKEUP_CFG;
        cc3200_reg_t NWP_LPDS_WAKEUP_SRC;
        cc3200_reg_t NWP_RESET_CAUSE;
        cc3200_reg_t NWP_LPDS_WAKETIME_OPP_CFG;
        cc3200_reg_t RESERVER31[1];
        cc3200_reg_t NWP_SRAM_DSLP_CFG;
        cc3200_reg_t NWP_SRAM_LPDS_CFG;
        cc3200_reg_t NWP_LPDS_WAKETIME_WAKE_CFG;
        cc3200_reg_t NWP_AUTONMS_SPI_MASTER_SEL;
        cc3200_reg_t NWP_AUTONMS_SPI_IDLE_REQ;
        cc3200_reg_t WLAN_TO_NWP_WAKE_REQUEST;
        cc3200_reg_t NWP_TO_WLAN_WAKE_REQUEST;
        cc3200_reg_t NWP_GPIO_WAKE_CONF;
        cc3200_reg_t GPRCM_EFUSE_READ_REG12;
        cc3200_reg_t RESERVER32[3];
        cc3200_reg_t GPRCM_DIEID_READ_REG5;
        cc3200_reg_t GPRCM_DIEID_READ_REG6;
        cc3200_reg_t RESERVER4[236];
        cc3200_reg_t REF_FSM_CFG0;
        cc3200_reg_t REF_FSM_CFG1;
        cc3200_reg_t APLLMCS_WLAN_CONFIG0_40;
        cc3200_reg_t APLLMCS_WLAN_CONFIG1_40;
        cc3200_reg_t APLLMCS_WLAN_CONFIG0_26;
        cc3200_reg_t APLLMCS_WLAN_CONFIG1_26;
        cc3200_reg_t APLLMCS_WLAN_OVERRIDES;
        cc3200_reg_t APLLMCS_MCU_RUN_CONFIG0_38P4;
        cc3200_reg_t APLLMCS_MCU_RUN_CONFIG1_38P4;
        cc3200_reg_t APLLMCS_MCU_RUN_CONFIG0_26;
        cc3200_reg_t APLLMCS_MCU_RUN_CONFIG1_26;
        cc3200_reg_t SPARE_RW0;
        cc3200_reg_t SPARE_RW1;
        cc3200_reg_t APLLMCS_MCU_OVERRIDES;
        cc3200_reg_t SYSCLK_SWITCH_STATUS;
        cc3200_reg_t REF_LDO_CONTROLS;
        cc3200_reg_t REF_RTRIM_CONTROL;
        cc3200_reg_t REF_SLICER_CONTROLS0;
        cc3200_reg_t REF_SLICER_CONTROLS1;
        cc3200_reg_t REF_ANA_BGAP_CONTROLS0;
        cc3200_reg_t REF_ANA_BGAP_CONTROLS1;
        cc3200_reg_t REF_ANA_SPARE_CONTROLS0;
        cc3200_reg_t REF_ANA_SPARE_CONTROLS1;
        cc3200_reg_t MEMSS_PSCON_OVERRIDES0;
        cc3200_reg_t MEMSS_PSCON_OVERRIDES1;
        cc3200_reg_t PLL_REF_LOCK_OVERRIDES;
        cc3200_reg_t MCU_PSCON_DEBUG;
        cc3200_reg_t MEMSS_PWR_PS;
        cc3200_reg_t REF_FSM_DEBUG;
        cc3200_reg_t MEM_SYS_OPP_REQ_OVERRIDE;
        cc3200_reg_t MEM_TESTCTRL_PD_OPP_CONFIG;
        cc3200_reg_t MEM_WL_FAST_CLK_REQ_OVERRIDES;
        cc3200_reg_t MEM_MCU_PD_MODE_REQ_OVERRIDES;
        cc3200_reg_t MEM_MCSPI_SRAM_OFF_REQ_OVERRIDES;
        cc3200_reg_t MEM_WLAN_APLLMCS_OVERRIDES;
        cc3200_reg_t MEM_REF_FSM_CFG2;
        cc3200_reg_t RESERVER5[224];
        cc3200_reg_t TESTCTRL_POWER_CTRL;
        cc3200_reg_t SSDIO_POWER_CTRL;
        cc3200_reg_t MCSPI_N1_POWER_CTRL;
        cc3200_reg_t WELP_POWER_CTRL;
        cc3200_reg_t WL_SDIO_POWER_CTRL;
        cc3200_reg_t WLAN_SRAM_ACTIVE_PWR_CFG;
        cc3200_reg_t RESERVER6[1];
        cc3200_reg_t WLAN_SRAM_SLEEP_PWR_CFG;
        cc3200_reg_t APPS_SECURE_INIT_DONE;
        cc3200_reg_t APPS_DEV_MODE_INIT_DONE;
        cc3200_reg_t EN_APPS_REBOOT;
        cc3200_reg_t MEM_APPS_PERIPH_PRESENT;
        cc3200_reg_t MEM_NWP_PERIPH_PRESENT;
        cc3200_reg_t MEM_SHARED_PERIPH_PRESENT;
        cc3200_reg_t NWP_PWR_STATE;
        cc3200_reg_t APPS_PWR_STATE;
        cc3200_reg_t MCU_PWR_STATE;
        cc3200_reg_t WTOP_PM_PS;
        cc3200_reg_t WTOP_PD_RESETZ_OVERRIDE_REG;
        cc3200_reg_t WELP_PD_RESETZ_OVERRIDE_REG;
        cc3200_reg_t WL_SDIO_PD_RESETZ_OVERRIDE_REG;
        cc3200_reg_t SSDIO_PD_RESETZ_OVERRIDE_REG;
        cc3200_reg_t MCSPI_N1_PD_RESETZ_OVERRIDE_REG;
        cc3200_reg_t TESTCTRL_PD_RESETZ_OVERRIDE_REG;
        cc3200_reg_t MCU_PD_RESETZ_OVERRIDE_REG;
        cc3200_reg_t RESERVER7[1];
        cc3200_reg_t GPRCM_EFUSE_READ_REG0;
        cc3200_reg_t GPRCM_EFUSE_READ_REG1;
        cc3200_reg_t GPRCM_EFUSE_READ_REG2;
        cc3200_reg_t GPRCM_EFUSE_READ_REG3;
        cc3200_reg_t WTOP_MEM_RET_CFG;
        cc3200_reg_t COEX_CLK_SWALLOW_CFG0;
        cc3200_reg_t COEX_CLK_SWALLOW_CFG1;
        cc3200_reg_t COEX_CLK_SWALLOW_CFG2;
        cc3200_reg_t COEX_CLK_SWALLOW_ENABLE;
        cc3200_reg_t DCDC_CLK_GEN_CONFIG;
        cc3200_reg_t GPRCM_EFUSE_READ_REG4;
        cc3200_reg_t GPRCM_EFUSE_READ_REG5;
        cc3200_reg_t GPRCM_EFUSE_READ_REG6;
        cc3200_reg_t GPRCM_EFUSE_READ_REG7;
        cc3200_reg_t GPRCM_EFUSE_READ_REG8;
        cc3200_reg_t GPRCM_EFUSE_READ_REG9;
        cc3200_reg_t GPRCM_EFUSE_READ_REG10;
        cc3200_reg_t GPRCM_EFUSE_READ_REG11;
        cc3200_reg_t GPRCM_DIEID_READ_REG0;
        cc3200_reg_t GPRCM_DIEID_READ_REG1;
        cc3200_reg_t GPRCM_DIEID_READ_REG2;
        cc3200_reg_t GPRCM_DIEID_READ_REG3;
        cc3200_reg_t GPRCM_DIEID_READ_REG4;
        cc3200_reg_t APPS_SS_OVERRIDES;
        cc3200_reg_t NWP_SS_OVERRIDES;
        cc3200_reg_t SHARED_SS_OVERRIDES;
        cc3200_reg_t IDMEM_CORE_RST_OVERRIDES;
        cc3200_reg_t TOP_DIE_FSM_OVERRIDES;
        cc3200_reg_t MCU_PSCON_OVERRIDES;
        cc3200_reg_t WTOP_PSCON_OVERRIDES;
        cc3200_reg_t WELP_PSCON_OVERRIDES;
        cc3200_reg_t WL_SDIO_PSCON_OVERRIDES;
        cc3200_reg_t MCSPI_PSCON_OVERRIDES;
        cc3200_reg_t SSDIO_PSCON_OVERRIDES;
    } cc3200_gprcm_t;
#define GPRCM_BASE 0x4402D000

#define GPRCM ((cc3200_gprcm_t *)GPRCM_BASE) /**< One and only instance of the G Power Control Module */

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* CC3200_PRCM_H */

/** @} */
/** @} */