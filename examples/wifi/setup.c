#include "driver.h"
#include "proto.h"
#include "utils.h"
#include "xtimer.h"

#include "vendor/hw_ints.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_types.h"
#include "vendor/hw_udma.h"
// TODO: hot candidate for replacement
#include "driverlib/interrupt.h"
#include "driverlib/pin.h"
#include "driverlib/prcm.h"
#include "driverlib/rom_map.h"
#include "driverlib/spi.h"
#include "driverlib/utils.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#define irq_isr_enter()                                                        \
  int _irq_state = irq_disable();                                              \
  // irq_interrupt_nesting++;

/** Macro that has to be used at the exit point of an ISR */
#define irq_isr_exit()                                                         \
  irq_restore(_irq_state);                                                     \
  if (sched_context_switch_request)                                            \
    thread_yield();

static bool wifiModuleBooted = false;

// wifi module register memory map
// volatile CC3200_MCSPI* wifiReg = (struct CC3200_MCSPI *)WIFI_SPI_BASE;

/**
 * @brief utility to get bit rate for a given rom minor version
 *
 */
uint32_t getSPIBitRate(uint8_t minorVer) {
  switch (minorVer) {
  case ROM_VER_PG1_21:
  case ROM_VER_PG1_32:
    return SPI_RATE_13M;
    break;
  case ROM_VER_PG1_33:
    return SPI_RATE_20M;
    break;
  default:
    return 0;
  }
}

static inline void maskWifiInterrupt(void) { HWREG(0x400F7088) = 0x1; }

int registerRxInterruptHandler(SimpleLinkEventHandler handler) {
  MAP_IntRegister(INT_NWPIC, handler);
  MAP_IntPrioritySet(INT_NWPIC, 0x20);
  MAP_IntPendClear(INT_NWPIC);
  MAP_IntEnable(INT_NWPIC);
  return 0;
}

void _clear_wifi_interrupt_handler(void) {
  MAP_IntDisable(INT_NWPIC);
  MAP_IntUnregister(INT_NWPIC);
  MAP_IntPendClear(INT_NWPIC);

  // TODO: also clear any IO specific parts
}

void powerOffWifi(void) {
  // must delay 300 usec to enable the NWP to finish all sl_stop activities
  UtilsDelay(300 * 80 / 3);

  // mask network processor interrupt interrupt
  maskWifiInterrupt();

  // Switch to PFM Mode
  HWREG(0x4402F024) &= 0xF7FFFFFF;

  // sl_stop eco for PG1.32 devices
  HWREG(0x4402E16C) |= 0x2;

  UtilsDelay(800000);
}

void powerOnWifi(void) {
  // bring the 1.32 eco out of reset
  HWREG(0x4402E16C) &= 0xFFFFFFFD;

  // NWP Wakeup
  HWREG(0x44025118) = 1;

  // UnMask Host interrupt
  (*(unsigned long *)REG_INT_MASK_CLR) = 0x1;
}

/**
 * @brief
 *
 */
void wifiRxHandler(void *value) {
  irq_isr_enter();
  if (value != NULL) {
    puts(value);
  }
  maskWifiInterrupt();

  wifiModuleBooted = true;

  irq_isr_exit();
}
/**
 * @brief
 *
 */
int initWifiModule(void) {
  powerOffWifi();

  // register callback when wifi module is powered back on
  registerRxInterruptHandler((SimpleLinkEventHandler)wifiRxHandler);

  powerOnWifi();

  // loop till the wifi module has booted
  while (!wifiModuleBooted) {
    xtimer_sleep(1);
  }
  cc3200_SlResponseHeader cmd;
  readCmdHeader(&cmd);

  // check for INITCOMPLETE opcode
  if (cmd.GenHeader.Opcode != 0x0008) {
    printf("ERR wifi module: invalid opcode %d received", cmd.GenHeader.Opcode);
    return -1;
  }

  return 0;
}

/**
 * @brief init wifi SPI and return FD
 *
 * @return int
 */
int initWifiSPI(void) {
  // TODO: later remove check when replaced calls
  // get rom version.
  uint32_t spiBitRate = 0;
  CC3200_RomInfo *romInfo = getDeviceRomInfo();

  MAP_PRCMPeripheralClkEnable(PRCM_LSPI, PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);

  // Disable Chip Select
  MAP_SPICSDisable(WIFI_SPI_BASE);

  // Disable SPI Channel
  MAP_SPIDisable(WIFI_SPI_BASE);

  // reset SPI
  MAP_SPIReset(WIFI_SPI_BASE);

  spiBitRate = getSPIBitRate(romInfo->minorVer);
  if (spiBitRate == 0) {
    return -1;
  }
  // NWP master interface
  // ulBase = LSPI_BASE;

  MAP_SPIConfigSetExpClk(WIFI_SPI_BASE, MAP_PRCMPeripheralClockGet(PRCM_LSPI),
                         spiBitRate, SPI_MODE_MASTER, SPI_SUB_MODE_0,
                         (SPI_SW_CTRL_CS | SPI_4PIN_MODE | SPI_TURBO_OFF |
                          SPI_CS_ACTIVEHIGH | SPI_WL_32));

  // if(MAP_PRCMPeripheralStatusGet(PRCM_UDMA))
  // {
  //   g_ucDMAEnabled = (HWREG(UDMA_BASE + UDMA_O_CTLBASE) != 0x0) ? 1 : 0;
  // }

  MAP_SPIEnable(WIFI_SPI_BASE);
  return 0;
}

int setupWifiModule(void) {
  sendPowerOnPreamble();
  if (initWifiSPI() != 0) {
    puts("failed to init wifi spi module");
    return -1;
  }
  if (initWifiModule() != 0) {
    puts("failed to start wifi module");
    return -1;
  }
  return 0;
}