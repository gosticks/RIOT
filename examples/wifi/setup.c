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
#include "periph/spi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

static bool wifiModuleBooted = false;
static uint8_t handledIrqsCount = 0;

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
    return SPI_CLK_13MHZ;
    break;
  case ROM_VER_PG1_33:
    return SPI_CLK_20MHZ;
    break;
  default:
    return 0;
  }
}

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
  unmaskWifiInterrupt();
}

static uint8_t sharedBuffer[1024];
typedef struct {
  _u8 connection_type; /* 0-STA,3-P2P_CL */
  _u8 ssid_len;
  _u8 ssid_name[32];
  _u8 go_peer_device_name_len;
  _u8 go_peer_device_name[32];
  _u8 bssid[6];
  _u8 reason_code;
  _u8 padding[2];
} slWlanConnectAsyncResponse_t;

typedef struct {
  _u32 ip;
  _u32 gateway;
  _u32 dns;
} SlIpV4AcquiredAsync_t;

void handleWlanConnectedResponse(void) {
  slWlanConnectAsyncResponse_t *test =
      (slWlanConnectAsyncResponse_t *)sharedBuffer;
  puts((char *)test->ssid_name);
}

void handleIpAcquired(void) {
  SlIpV4AcquiredAsync_t *test = (SlIpV4AcquiredAsync_t *)sharedBuffer;
  uint8_t *octets = (uint8_t *)&test->ip;
  printf("IP %u.%u.%u.%u", octets[3], octets[2], octets[1], octets[0]);
}

void defaultCommandHandler(cc3200_SlResponseHeader *header) {
  // printf("GOT RESPONSE: opcode=%d", header->GenHeader.Opcode);
  read(sharedBuffer, header->GenHeader.Len);
  switch (header->GenHeader.Opcode) {
  case SL_OPCODE_WLAN_WLANASYNCCONNECTEDRESPONSE:
    handleWlanConnectedResponse();
    break;
  case SL_OPCODE_NETAPP_IPACQUIRED:
    handleIpAcquired();
    break;
  }
  unmaskWifiInterrupt();
}

/**
 * @brief
 *
 */
void wifiRxHandler(void *value) {
  if (value != NULL) {
    puts(value);
  }
  handledIrqsCount++;
  maskWifiInterrupt();
  // initWifiModule is waiting for the setup command so
  // only use default handler after that
  if (wifiModuleBooted) {
    cc3200_SlResponseHeader cmd;
    readCmdHeader(&cmd);
    defaultCommandHandler(&cmd);
  } else {
    wifiModuleBooted = true;
  }

  cortexm_isr_end();
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
  unmaskWifiInterrupt();
  // check for INITCOMPLETE opcode
  if (cmd.GenHeader.Opcode != 0x0008) {
    printf("ERR wifi module: invalid opcode %d received", cmd.GenHeader.Opcode);
    return -1;
  }

  // wait till device is ready for commands
  while (cmd.GenHeader.Opcode != 0x0063) {
    readCmdHeader(&cmd);
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