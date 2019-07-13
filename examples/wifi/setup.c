#include "cmd.h"
#include "driver.h"
#include "proto.h"
#include "state.h"
#include "utils.h"
#include "xtimer.h"

#include "vendor/hw_ints.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_types.h"
#include "vendor/hw_udma.h"
#include "vendor/rom.h"
#include "vendor/rom_map.h"
// TODO: hot candidate for replacement
// #include "driverlib/interrupt.h"
// #include "driverlib/pin.h"
// #include "driverlib/prcm.h"
// #include "driverlib/spi.h"
// #include "driverlib/utils.h"
#include "periph/spi.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

// static bool wifiModuleBooted = false;
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
  ROM_IntRegister(INT_NWPIC, handler);
  NVIC_SetPriority(INT_NWPIC, 0x20);
  NVIC_ClearPendingIRQ(INT_NWPIC);
  NVIC_EnableIRQ(INT_NWPIC);
  // ROM_IntEnable(INT_NWPIC);
  return 0;
}

void _clear_wifi_interrupt_handler(void) {
  NVIC_DisableIRQ(INT_NWPIC);
  // ROM_IntDisable(INT_NWPIC);
  ROM_IntUnregister(INT_NWPIC);
  NVIC_ClearPendingIRQ(INT_NWPIC);

  // TODO: also clear any IO specific parts
}

void powerOffWifi(void) {
  // must delay 300 usec to enable the NWP to finish all sl_stop activities
  delay(300 * 80 / 3);

  // mask network processor interrupt interrupt
  maskWifiInterrupt();

  // Switch to PFM Mode
  HWREG(0x4402F024) &= 0xF7FFFFFF;

  // sl_stop eco for PG1.32 devices
  HWREG(0x4402E16C) |= 0x2;

  delay(800000);
}

void powerOnWifi(void) {
  // bring the 1.32 eco out of reset
  HWREG(0x4402E16C) &= 0xFFFFFFFD;

  // NWP Wakeup
  HWREG(0x44025118) = 1;

  // UnMask Host interrupt
  unmaskWifiInterrupt();
}

static uint8_t sharedBuffer[512];
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
  printf("[WIFI] connected to SSID ");
  printChars((char *)test->ssid_name, test->ssid_len);
  printf("\n");

  // store values on the current state
  state.con.type = test->connection_type;
  state.con.ssidLen = test->ssid_len;

  // copy ssid
  memcpy(state.con.ssid, test->ssid_name, test->ssid_len);

  // copy bssid
  memcpy(state.con.bssid, test->bssid, 6);

  state.con.connected = true;
}

void handleIpAcquired(void) {
  SlIpV4AcquiredAsync_t *test = (SlIpV4AcquiredAsync_t *)sharedBuffer;
  uint8_t *octets = (uint8_t *)&test->ip;
  printf("[WIFI] acquired IP %u.%u.%u.%u \n", octets[3], octets[2], octets[1],
         octets[0]);
}

void defaultCommandHandler(cc3200_SlResponseHeader *header) {
  printf("[WIFI] RECV: \033[1;32m%x \033[0m, len=%d\n",
         header->GenHeader.Opcode, header->GenHeader.Len);

  volatile DriverRequest *req = NULL;

  // check if any command is waiting on a response if so we can use
  // its buffer to avoid having to copy the data
  for (uint8_t i = 0; state.curReqCount != 0 && i < REQUEST_QUEUE_SIZE; i++) {
    if (state.requestQueue[i] == NULL ||
        state.requestQueue[i]->Opcode != header->GenHeader.Opcode) {
      continue;
    }
    req = state.requestQueue[i];
    req->Waiting = false;
    removeFromQueue(state.requestQueue[i]);
  }

  // when we have a request read the buffer to the request
  if (req != NULL) {
    int16_t remainder = header->GenHeader.Len - req->DescBufferSize;
    if (remainder < 0) {
      remainder = 0;
    }
    if (req->DescBufferSize > 0 && remainder >= 0) {
      read(req->DescBuffer, req->DescBufferSize);
    }

    // payload can sometimes be smaller then expected
    if (remainder < req->PayloadBufferSize) {
      read(req->PayloadBuffer, remainder);
      remainder = 0;
    } else {
      remainder -= req->PayloadBufferSize;

      if (req->PayloadBufferSize > 0 && remainder >= 0) {
        read(req->PayloadBuffer, req->PayloadBufferSize);
      }
    }

    // read all remaining data
    if (remainder > 0) {
      read(sharedBuffer, remainder);
    }
  } else {
    // otherwise read everything into shared buffer;
    read(sharedBuffer, header->GenHeader.Len);
  }

  // handle commands
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
  (void)value;
  // if (value != NULL) {
  //   puts(value);
  // }
  handledIrqsCount++;
  maskWifiInterrupt();
  // initWifiModule is waiting for the setup command so
  // only use default handler after that

  cc3200_SlResponseHeader cmd;
  readCmdHeader(&cmd);
  defaultCommandHandler(&cmd);

  cortexm_isr_end();
}
/**
 * @brief
 *
 */
int initWifiModule(void) {
  // register callback when wifi module is powered back on
  registerRxInterruptHandler((SimpleLinkEventHandler)wifiRxHandler);

  powerOnWifi();

  volatile DriverRequest r = {
      .Opcode = 0x0008, .Waiting = true, .DescBufferSize = 0};
  addToQueue(&r);

  while (r.Waiting) {
  }
  puts("[WIFI] setup completed");
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

  // MAP_PRCMPeripheralClkEnable(PRCM_LSPI, PRCM_RUN_MODE_CLK |
  // PRCM_SLP_MODE_CLK);

  // // Disable Chip Select
  // MAP_SPICSDisable(WIFI_SPI_BASE);

  // // Disable SPI Channel
  // MAP_SPIDisable(WIFI_SPI_BASE);

  // // reset SPI
  // MAP_SPIReset(WIFI_SPI_BASE);

  spiBitRate = getSPIBitRate(romInfo->minorVer);
  if (spiBitRate == 0) {
    return -1;
  }
  // NWP master interface
  // ulBase = LSPI_BASE;

  // MAP_SPIConfigSetExpClk(WIFI_SPI_BASE,
  // MAP_PRCMPeripheralClockGet(PRCM_LSPI),
  //                        spiBitRate, SPI_MODE_MASTER, SPI_SUB_MODE_0,
  //                        (SPI_SW_CTRL_CS | SPI_4PIN_MODE | SPI_TURBO_OFF |
  //                         SPI_CS_ACTIVEHIGH | SPI_WL_32));

  // TODO: add UDMA to improve transmission performance
  // if(MAP_PRCMPeripheralStatusGet(PRCM_UDMA))
  // {
  //   g_ucDMAEnabled = (HWREG(UDMA_BASE + UDMA_O_CTLBASE) != 0x0) ? 1 : 0;
  // }

  // MAP_SPIEnable(WIFI_SPI_BASE);

  spi_init(1);
  spi_acquire(1, 0, SPI_SUB_MODE_0, spiBitRate);

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
  // 2 = AP mode, 0 = Station
  if (setWifiMode(0) != 0) {
    puts("failed to set wifi mode");
    return -1;
  }

  // getDevice mac address

  // get mac address
  getNetConfig(SL_MAC_ADDRESS_GET, NULL, 6, (unsigned char *)&state.macAddr);

  // sendPowerOnPreamble();
  // if (initWifiModule() != 0) {
  //   puts("failed to start wifi module");
  //   return -1;
  // }
  return 0;
}