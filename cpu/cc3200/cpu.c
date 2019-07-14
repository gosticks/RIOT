#include "cpu.h"
#include "periph/init.h"
#include "periph_cpu.h"
#include "vendor/hw_hib3p3.h"
#include "vendor/hw_ints.h"
#include "vendor/hw_types.h"

/**
 * @brief wait for 3 * count cpu cicles
 *
 */
void __attribute__((naked)) delay(unsigned long count) {
  __asm("    subs    r0, #1\n"
        "    bne     delay\n"
        "    bx      lr");
}

/**
 * @brief init peripheria clock and perform a softreset
 *
 * @param reg pointer to register
 */
void init_periph_clk(cc3200_periph_regs_t *reg) {
  volatile unsigned long ulDelay;

  // enable & reset periphiria
  reg->soft_reset |= PRCM_SOFT_RESET;

  // wait for the hardware to perform reset
  for (ulDelay = 0; ulDelay < 16; ulDelay++) {
  }

  // deassert reset
  reg->soft_reset &= ~PRCM_SOFT_RESET;
}

uint32_t get_sys_reset_cause(void) {
  uint32_t reset_cause;
  uint32_t hiber_status;
  // reset read reset status
  reset_cause = GPRCM->APPS_RESET_CAUSE & 0xFF;

  if (reset_cause == PRCM_POWER_ON) {
    hiber_status = HWREG(HIB3P3_BASE + HIB3P3_O_MEM_HIB_WAKE_STATUS);
    // FIXME: wait for some reason test if this is needed
    delay((80 * 200) / 3);
    if (hiber_status & 0x1) {
      return PRCM_HIB_EXIT;
    }
  }
  return reset_cause;
}

void system_init(void) {
  cc3200_reg_t tmp;

  // DIG DCDC LPDS ECO Enable
  HWREG(0x4402F064) |= 0x800000;

  // enable hibernate ECO
  tmp = HWREG(HIB3P3_BASE + HIB3P3_O_MEM_HIB_WAKE_STATUS);
  delay((80 * 200) / 3);
  HWREG(HIB3P3_BASE + HIB3P3_O_MEM_HIB_REG0) = tmp | (1 << 4);
  delay((80 * 200) / 3);

  // enable clock switching
  HWREG(0x4402E16C) |= 0x3C;

  // enable and reset default periphiria
  init_periph_clk(&ARCM->UDMA_A);
  // disable udma
  ARCM->UDMA_A.clk_gating &= ~PRCM_RUN_MODE_CLK;

  if (get_sys_reset_cause() == PRCM_POWER_ON) {
    HWREG(0x4402F804) = 0x1;
    delay((80 * 200) / 3);
  }

  //
  // SWD mode
  //
  if (((HWREG(0x4402F0C8) & 0xFF) == 0x2)) {
    HWREG(0x4402E110) = ((HWREG(0x4402E110) & ~0xC0F) | 0x2);
    HWREG(0x4402E114) = ((HWREG(0x4402E110) & ~0xC0F) | 0x2);
  }

  //
  // Override JTAG mux
  //
  HWREG(0x4402E184) |= 0x2;

  //
  // Change UART pins(55,57) mode to PIN_MODE_0 if they are in PIN_MODE_1
  //
  if ((HWREG(0x4402E0A4) & 0xF) == 0x1) {
    HWREG(0x4402E0A4) = ((HWREG(0x4402E0A4) & ~0xF));
  }

  if ((HWREG(0x4402E0A8) & 0xF) == 0x1) {
    HWREG(0x4402E0A8) = ((HWREG(0x4402E0A8) & ~0xF));
  }

  //
  // DIG DCDC VOUT trim settings based on PROCESS INDICATOR
  //
  if (((HWREG(0x4402DC78) >> 22) & 0xF) == 0xE) {
    HWREG(0x4402F0B0) = ((HWREG(0x4402F0B0) & ~(0x00FC0000)) | (0x32 << 18));
  } else {
    HWREG(0x4402F0B0) = ((HWREG(0x4402F0B0) & ~(0x00FC0000)) | (0x29 << 18));
  }

  //
  // Enable SOFT RESTART in case of DIG DCDC collapse
  //
  HWREG(0x4402FC74) &= ~(0x10000000);

  //
  // Disable the sleep for ANA DCDC
  //
  HWREG(0x4402F0A8) |= 0x00000004;
}

void cpu_init(void) {
  /* initializes the Cortex-M core */
  cortexm_init();

  /* init board */
  system_init();

  /* trigger static peripheral initialization */
  periph_init();
}