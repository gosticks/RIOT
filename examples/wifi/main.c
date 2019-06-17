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
// #include "board.h"

#include "driver.h"
#include "driverlib/rom_map.h"
#include "periph/cpuid.h"
#include "periph/gpio.h"
#include "proto.h"
#include "setup.h"
#include "utils.h"
#include "xtimer.h"

const WifiCtrlCmd DeviceGetCommand = {
    0x8466, // SL_OPCODE_DEVICE_DEVICEGET,
    sizeof(cc3200_DeviceSetGet_t),
    sizeof(cc3200_DeviceSetGet_t),
};

// unsigned char g_ucDMAEnabled = 0;
void test_write(void) {
  // uint16_t ucConfigOpt = 12;

  // short sync pattern for SPI based connection
  // _write_sync_patter();

  // write command header
  uint16_t len = alignDataLen(
      DeviceGetCommand.TxDescLen); // TODO: also add payload size here
  sendHeader(DeviceGetCommand.Opcode, len);

  // SlVersionFull ver;

  // cc3200_DeviceMsgGet_u Msg;
  // Msg.Cmd.DeviceSetId = 1;
  // Msg.Cmd.Option = ucConfigOpt;

  // now read
  sendShortSync();

  // reset read word count
  MAP_SPIWordCountSet(WIFI_SPI_BASE, 0);

  uint8_t buf[sizeof(cc3200_SlResponseHeader)];

  // read 4 bytes into buffer
  read(buf, 4);
  // write content
  // send();
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
      xtimer_sleep(1);
      gpio_toggle(LED_RED);
    }
  }
  gpio_clear(LED_RED);
  xtimer_sleep(1);
  gpio_toggle(LED_GREEN);
  xtimer_sleep(1);
  gpio_toggle(LED_GREEN);
  xtimer_sleep(1);
  gpio_toggle(LED_GREEN);

  test_write();

  // WifiModule t = {.fd = fd};
  // test = &t;
}

int main(void) {
  uint8_t cpuid[CPUID_LEN];
  cpuid_get(cpuid);
  printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
  init_wifi();

  // keept the programm going
  while (1) {
  }

  return 0;
}