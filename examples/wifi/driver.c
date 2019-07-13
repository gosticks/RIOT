#include "vendor/hw_ints.h"
#include "vendor/hw_mcspi.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_types.h"
#include "vendor/hw_udma.h"

// #include "cc3200_spi.h"
#include "periph/spi.h"
// #include "driverlib/interrupt.h"
// #include "driverlib/pin.h"
// #include "driverlib/prcm.h"
// #include "driverlib/rom_map.h"
// #include "driverlib/spi.h"
// #include "driverlib/utils.h"

#include "proto.h"
#include "protocol.h"
#include "setup.h"
#include "utils.h"

#include <stdbool.h>
#include <stdio.h>

// static volatile cc3200_spi_t *wifiReg = (struct cc3200_spi_t *)WIFI_SPI_BASE;
const _SlSyncPattern_t g_H2NSyncPattern = CPU_TO_NET_CHIP_SYNC_PATTERN;
const _SlSyncPattern_t g_H2NCnysPattern = CPU_TO_NET_CHIP_CNYS_PATTERN;
static uint32_t TxSeqNum = 0;

int read(uint8_t *buf, int len) {
  spi_transfer_bytes(1, SPI_CS_UNDEF, true, NULL, buf, len);
  // unsigned long ulCnt;
  // unsigned long *ulDataIn;

  // MAP_SPICSEnable((int)wifiReg);

  // //
  // // Initialize local variable.
  // //
  // ulDataIn = (unsigned long *)buf;
  // ulCnt = (len + 3) >> 2;

  // //
  // // Reading loop
  // //
  // while (ulCnt--) {
  //   while (!(wifiReg->ch0_stat & MCSPI_CH0STAT_TXS))
  //     ;
  //   wifiReg->tx0 = 0xFFFFFFFF;
  //   while (!(wifiReg->ch0_stat & MCSPI_CH0STAT_RXS))
  //     ;
  //   *ulDataIn = wifiReg->rx0;
  //   ulDataIn++;
  // }

  // MAP_SPICSDisable((int)wifiReg);

  return len;
}

int send(uint8_t *in, int len) {
  spi_transfer_bytes(1, SPI_CS_UNDEF, true, in, NULL, len);
  // unsigned long ulCnt;
  // unsigned long *ulDataOut;
  // unsigned long ulDataIn = 0;

  // // enable spi
  // MAP_SPICSEnable(WIFI_SPI_BASE);

  // ulDataOut = (unsigned long *)in;
  // ulCnt = (len + 3) >> 2;

  // //
  // // Writing Loop
  // //
  // while (ulCnt--) {
  //   // send one word of data
  //   while (!(wifiReg->ch0_stat & MCSPI_CH0STAT_TXS))
  //     ;
  //   wifiReg->tx0 = *ulDataOut;

  //   // read one word of response
  //   while (!(wifiReg->ch0_stat & MCSPI_CH0STAT_RXS))
  //     ;
  //   ulDataIn = wifiReg->rx0;

  //   // increment pointers
  //   ulDataOut++;
  // }
  // (void)ulDataIn;

  // // disable spi again
  // MAP_SPICSDisable(WIFI_SPI_BASE);

  return len;
}

int _readCmdHeader(uint8_t *buf, uint8_t *align) {
  // write sync
  send((uint8_t *)&g_H2NCnysPattern.Short, sizeof(uint32_t));

  uint32_t SyncCnt = 0;

  // read 4 bytes
  read(buf, 4);
  while (!N2H_SYNC_PATTERN_MATCH(buf, TxSeqNum)) {
    if (0 == (SyncCnt % (uint32_t)4)) {
      read(&buf[4], 4);
    }

    sliceFirstInBuffer(buf, 8);
    SyncCnt++;
  }

  SyncCnt %= 4;

  if (SyncCnt > 0) {
    *(uint32_t *)buf = *(uint32_t *)&buf[4];
    read(&buf[4 - SyncCnt], (uint16_t)SyncCnt);
  } else {
    read(buf, 4);
  }

  while (N2H_SYNC_PATTERN_MATCH(buf, TxSeqNum)) {
    read(buf, 4);
  }
  TxSeqNum++;

  /*  7. Here we've read Generic Header (4 bytes). Read the Resp Specific header
   * (4 more bytes). */
  read(&buf[4], 4);
  *align = (uint8_t)((SyncCnt > 0) ? (4 - SyncCnt) : 0);
  return 0;
}

int readCmdHeader(cc3200_SlResponseHeader *resp) {
  uint8_t align;
  _readCmdHeader((uint8_t *)resp, &align);

  // read up to the next aligned byte
  read((uint8_t *)&resp, align);
  return 0;
}

void sendShortSync(void) {
  send((uint8_t *)&g_H2NSyncPattern.Short, sizeof(uint32_t));
}

void sendHeader(_SlGenericHeader_t *header) {
  send((uint8_t *)header, sizeof(_SlGenericHeader_t));
}

void sendPowerOnPreamble(void) {
  unsigned int sl_stop_ind, apps_int_sts_raw, nwp_lpds_wake_cfg;
  unsigned int retry_count;
  /* Perform the sl_stop equivalent to ensure network services
     are turned off if active */
  HWREG(0x400F70B8) = 1; /* APPs to NWP interrupt */
  delay(800000 / 5);

  retry_count = 0;
  nwp_lpds_wake_cfg = HWREG(0x4402D404);
  sl_stop_ind = HWREG(0x4402E16C);

  if ((nwp_lpds_wake_cfg != 0x20) && /* Check for NWP POR condition */
      !(sl_stop_ind & 0x2))          /* Check if sl_stop was executed */
  {
    /* Loop until APPs->NWP interrupt is cleared or timeout */
    while (retry_count < 1000) {
      apps_int_sts_raw = HWREG(0x400F70C0);
      if (apps_int_sts_raw & 0x1) {
        delay(800000 / 5);
        retry_count++;
      } else {
        break;
      }
    }
  }
  HWREG(0x400F70B0) = 1; /* Clear APPs to NWP interrupt */
  delay(800000 / 5);

  powerOffWifi();
}