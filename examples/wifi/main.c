/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */

#include <stdio.h>
#include <string.h>
// #include "board.h"

#include "cmd.h"
#include "driver.h"
#include "driverlib/rom_map.h"
#include "periph/cpuid.h"
#include "periph/gpio.h"
#include "proto.h"
#include "setup.h"
#include "state.h"
#include "utils.h"

#include "driverlib/utils.h"

#define SL_RAW_RF_TX_PARAMS_CHANNEL_SHIFT (0)
#define SL_RAW_RF_TX_PARAMS_RATE_SHIFT (6)
#define SL_RAW_RF_TX_PARAMS_POWER_SHIFT (11)
#define SL_RAW_RF_TX_PARAMS_PREAMBLE_SHIFT (15)

#define SL_RAW_RF_TX_PARAMS(chan, rate, power, preamble)                       \
  ((chan << SL_RAW_RF_TX_PARAMS_CHANNEL_SHIFT) |                               \
   (rate << SL_RAW_RF_TX_PARAMS_RATE_SHIFT) |                                  \
   (power << SL_RAW_RF_TX_PARAMS_POWER_SHIFT) |                                \
   (preamble << SL_RAW_RF_TX_PARAMS_PREAMBLE_SHIFT))

#define SL_POLICY_CONNECTION (0x10)
#define SL_POLICY_SCAN (0x20)
#define SL_POLICY_PM (0x30)
#define SL_POLICY_P2P (0x40)

#define VAL_2_MASK(position, value) ((1 & (value)) << (position))
#define MASK_2_VAL(position, mask) (((1 << position) & (mask)) >> (position))

#define SL_CONNECTION_POLICY(Auto, Fast, Open, anyP2P, autoSmartConfig)        \
  (VAL_2_MASK(0, Auto) | VAL_2_MASK(1, Fast) | VAL_2_MASK(2, Open) |           \
   VAL_2_MASK(3, anyP2P) | VAL_2_MASK(4, autoSmartConfig))
#define SL_SCAN_POLICY_EN(policy) (MASK_2_VAL(0, policy))
#define SL_SCAN_POLICY(Enable) (VAL_2_MASK(0, Enable))

#define SL_NORMAL_POLICY (0)
#define SL_LOW_LATENCY_POLICY (1)
#define SL_LOW_POWER_POLICY (2)
#define SL_ALWAYS_ON_POLICY (3)
#define SL_LONG_SLEEP_INTERVAL_POLICY (4)

#define LED_RED GPIO_PIN(1, 9)
#define LED_ORANGE GPIO_PIN(1, 10)
#define LED_GREEN GPIO_PIN(1, 11)

const WifiCtrlCmd DeviceGetCommand = {
    0x8466, // SL_OPCODE_DEVICE_DEVICEGET,
    sizeof(_DeviceSetGet_t),
    sizeof(_DeviceSetGet_t),
};

char RawData_Ping[] = {
    /*---- wlan header start -----*/
    0x88, /* version , type sub type */
    0x02, /* Frame control flag */
    0x2C, 0x00, 0x00, 0x23, 0x75, 0x55, 0x55, 0x55, /* destination */
    0x00, 0x22, 0x75, 0x55, 0x55, 0x55,             /* bssid */
    0x08, 0x00, 0x28, 0x19, 0x02, 0x85,             /* source */
    0x80, 0x42, 0x00, 0x00, 0xAA, 0xAA, 0x03, 0x00, 0x00, 0x00, 0x08,
    0x00, /* LLC */
    /*---- ip header start -----*/
    0x45, 0x00, 0x00, 0x54, 0x96, 0xA1, 0x00, 0x00, 0x40, 0x01, 0x57,
    0xFA,                   /* checksum */
    0xc0, 0xa8, 0x01, 0x64, /* src ip */
    0xc0, 0xa8, 0x01, 0x02, /* dest ip  */
    /* payload - ping/icmp */
    0x08, 0x00, 0xA5, 0x51, 0x5E, 0x18, 0x00, 0x00, 0x41, 0x08, 0xBB, 0x8D,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// unsigned char g_ucDMAEnabled = 0;
void logHardwareVersion(void) {
  SlVersionFull ver = {0};
  getDeviceInfo(&ver);
  printf("[WIFI] CC3100 Build Version "
         "%li.%li.%li.%li.31.%li.%li.%li.%li.%i.%i.%i.%i\n\r",
         ver.NwpVersion[0], ver.NwpVersion[1], ver.NwpVersion[2],
         ver.NwpVersion[3], ver.ChipFwAndPhyVersion.FwVersion[0],
         ver.ChipFwAndPhyVersion.FwVersion[1],
         ver.ChipFwAndPhyVersion.FwVersion[2],
         ver.ChipFwAndPhyVersion.FwVersion[3],
         ver.ChipFwAndPhyVersion.PhyVersion[0],
         ver.ChipFwAndPhyVersion.PhyVersion[1],
         ver.ChipFwAndPhyVersion.PhyVersion[2],
         ver.ChipFwAndPhyVersion.PhyVersion[3]);
}

/**
 * @brief prepare wifi module to be operational
 *
 */
void init_wifi(void) {
  gpio_set(LED_RED);
  if (setupWifiModule() != 0) {
    // loop and blink to indicate problem (also helps with debugging)
    while (1) {
      UtilsDelay(80 * 50);
      gpio_toggle(LED_RED);
    }
  }
  gpio_clear(LED_RED);
  UtilsDelay(300000 * 80 / 3);
  gpio_set(LED_GREEN);
  UtilsDelay(300000 * 80 / 3);
  gpio_clear(LED_GREEN);
  UtilsDelay(300000 * 80 / 3);
  gpio_toggle(LED_GREEN);

  logHardwareVersion();

  // WifiModule t = {.fd = fd};
  // test = &t;
}

#define SL_MAC_ADDRESS_GET 2

void printMacAddr(void) {

  unsigned char macAddr[6];

  // get mac address
  getNetConfig(SL_MAC_ADDRESS_GET, NULL, 6, macAddr);

  printf("MAC ADDR  ");
  printf("%x:", macAddr[0]);
  printf("%x:", macAddr[1]);
  printf("%x:", macAddr[2]);
  printf("%x:", macAddr[3]);
  printf("%x:", macAddr[4]);
  printf("%x", macAddr[5]);
  printf("\n");
}

/* sockopt */
typedef _u32 SlTime_t;
typedef _u32 SlSuseconds_t;

typedef struct SlTimeval_t {
  SlTime_t tv_sec;       /* Seconds      */
  SlSuseconds_t tv_usec; /* Microseconds */
} SlTimeval_t;

typedef struct wifi_80211_baseheader {
  uint16_t fc;      // frame control
  uint16_t DirID;   // Diration ID
  uint8_t addr0[6]; // Diration ID
  uint8_t addr1[6]; // Diration ID
  uint8_t addr2[6]; // Diration ID
  uint16_t sc;      // SC
  uint8_t addr3[6]; // Diration ID
} wifi_80211_baseheader;

char acBuffer[1500];

int main(void) {
  uint8_t cpuid[CPUID_LEN];
  // unsigned char ucVal = 0;

  cpuid_get(cpuid);
  printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
  init_wifi();
  puts("wifi init completed !");
  printMacAddr();

  if (disconnectFromWifi() != 0) {
    puts("[WIFI] failed to disconnect");
  } else {
    puts("[WIFI] disconnected");
  }
  if (setWifiPolicy(SL_POLICY_SCAN, SL_SCAN_POLICY(0)) != 0) {
    puts("[WIFI] failed to set policy");
  } else {
    puts("[WIFI] policy set");
  }
  setWifiPolicy(SL_POLICY_CONNECTION, SL_CONNECTION_POLICY(0, 0, 0, 0, 0));

  // deleteProfile(0xFF);

  disconnectFromWifi();

  // disable DHCP
  // setNetConfig(4, 1, 1, &ucVal);

  // disable scan
  if (setWifiPolicy(SL_POLICY_SCAN, SL_SCAN_POLICY(0)) != 0) {
    puts("[WIFI] failed to set wifi policy");
  }

  // setup wifi power
  // power is a reverse metric (dB)
  uint8_t wifiPower = 0;
  setWifiConfig(1, 10, 1, &wifiPower);

  // connect(&apConf);
  // keept the programm going

  // wait for a connection
  // while (state.con.connected == 0) {
  //   puts("waiting for connection");
  //   UtilsDelay(30000 * 80 / 3);
  // }
  puts("Connection established");

  struct SlTimeval_t timeval;

  timeval.tv_sec = 0;      // Seconds
  timeval.tv_usec = 20000; // Microseconds.

  // get a socket
  // SL_AF_PACKET
  int16_t sock0 = openSocket(6, 2, 13);
  printf("Got socket %i \n", sock0);
  // char *buf = "HELLO WORLD";
  setSocketOptions(sock0, 1, 20, &timeval, sizeof(timeval));

  uint8_t filterConfig[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
  // reset rx filters
  setWlanFilter(1, filterConfig, sizeof(_WlanRxFilterOperationCommandBuff_t));

  while (1) {
    // sendRawTraceiverData(sock0, (uint8_t *)RawData_Ping, sizeof(RawData_Ping), SL_RAW_RF_TX_PARAMS(13, 1, 0, 0));
    recvRawTraceiverData(sock0, acBuffer, 1470, 0);
    // read the buffer
    for(int i = 0; i < 50; i++) {
      printf("%02x ", acBuffer[i]);
    }
    puts("");
    wifi_80211_baseheader *test = (wifi_80211_baseheader *)(acBuffer + 8);
    printf("GOT PACKET  ");
    printf("%02x:", test->addr3[0]);
    printf("%02x:", test->addr3[1]);
    printf("%02x:", test->addr3[2]);
    printf("%02x:", test->addr3[3]);
    printf("%02x:", test->addr3[4]);
    printf("%02x", test->addr3[5]);
    printf("\n");
    UtilsDelay(4000000);
    gpio_toggle(LED_GREEN);
    gpio_toggle(LED_ORANGE);
    gpio_toggle(LED_RED);
  }

  return 0;
}