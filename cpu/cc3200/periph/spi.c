/*
 * Copyright (C) 2015 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser General
 * Public License v2.1. See the file LICENSE in the top level directory for more
 * details.
 */

/**
 * @ingroup     cpu_cc3200
 * @{
 *
 * @file
 * @brief       Low-level SPI driver implementation
 *
 * @author      Attilio Dona' <@attiliodona>
 *
 * @}
 */

#include "periph/spi.h"
#include "board.h"
#include "cpu.h"
#include "mutex.h"
#include "periph/gpio.h"
#include "periph_conf.h"

#include "driverlib/pin.h"
#include "driverlib/prcm.h"
#include "driverlib/spi.h"
#include "driverlib/utils.h"
#include "vendor/hw_ints.h"
#include "vendor/hw_udma.h"

#define ENABLE_DEBUG (0)
#include "debug.h"
/**
 * @brief   Default SPI device access macro
 */
#ifndef SPI_DEV
#define SPI_DEV(x) (x)
#endif

// #define SPI_NUMOF 2U
#define EXTERNAL_SPI 0U
#define CC3100_SPI 1U

static const spi_conf_t spi_config[] = {
    {
        .base_addr = GSPI_BASE,
        .gpio_port = 0,
        .pins =
            (spi_pins_t){
                .miso = PIN_06, .sck = PIN_05, .mosi = PIN_07, .cs = PIN_08},
    },
    {.base_addr = LSPI_BASE, .gpio_port = 1}};

/**
 * @brief   Allocate one lock per SPI device
 */
static mutex_t locks[SPI_NUMOF];

void spi_init_pins(spi_t bus) {

  switch (bus) {
  case EXTERNAL_SPI:
    // enable peripherial clock
    MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);
    // TODO: use gpio_init for this when PIN_MODE mapping is done
    // GSPI_CLK
    MAP_PinTypeSPI(spi_config[bus].pins.sck, PIN_MODE_7);
    // set MISO pin
    MAP_PinTypeSPI(spi_config[bus].pins.miso, PIN_MODE_7);
    // set GSPI_MOSI
    MAP_PinTypeSPI(spi_config[bus].pins.mosi, PIN_MODE_7);
    // set GSPI_CS
    MAP_PinTypeSPI(spi_config[bus].pins.cs, PIN_MODE_7);

    break;
  case CC3100_SPI:
    // no setup is required for the wifi spi since the pins cannot be reset
    // (except for UART but that is not supported by design)
    break;
  }
}

void enable_peiph_clk(unsigned long bus, unsigned long mask) {
  MAP_PRCMPeripheralClkEnable(bus, mask);
}

void spi_reset(spi_t bus) {
  // Disable Chip Select
  MAP_SPICSDisable(spi_config[bus].base_addr);

  // Disable SPI Channel
  MAP_SPIDisable(spi_config[bus].base_addr);

  // reset SPI
  MAP_SPIReset(spi_config[bus].base_addr);

  MAP_SPIEnable(spi_config[bus].base_addr);
}

void spi_init(spi_t bus) {
  assert(bus < SPI_NUMOF);

  mutex_init(&locks[bus]);
  // trigger pin initialization
  spi_init_pins(bus);

  // enable clock
  switch (bus) {
  case EXTERNAL_SPI:
    enable_peiph_clk(PRCM_GSPI, PRCM_RUN_MODE_CLK);
    break;
  case CC3100_SPI:
    enable_peiph_clk(PRCM_LSPI, PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK);
    break;
  }

  // reset spi for the changes to take effect
  spi_reset(bus);
}

int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk) {
  /* lock bus */
  mutex_lock(&locks[bus]);

  if (bus >= SPI_UNDEF)
    return -1;

  // TODO: use cs mode and clock for now only master is supported with the
  // default clocks enable clock
  switch (bus) {
  case EXTERNAL_SPI:
    MAP_SPIConfigSetExpClk(spi_config[bus].base_addr,
                           MAP_PRCMPeripheralClockGet(PRCM_GSPI), clk,
                           SPI_MODE_MASTER, mode,
                           (SPI_HW_CTRL_CS | SPI_4PIN_MODE | SPI_TURBO_OFF |
                            SPI_CS_ACTIVELOW | SPI_WL_8));
    break;
  case CC3100_SPI:
    MAP_SPIConfigSetExpClk(spi_config[bus].base_addr,
                           MAP_PRCMPeripheralClockGet(PRCM_LSPI), clk,
                           SPI_MODE_MASTER, mode,
                           (SPI_SW_CTRL_CS | SPI_4PIN_MODE | SPI_TURBO_OFF |
                            SPI_CS_ACTIVEHIGH | SPI_WL_32));
    break;
  }
  return 0;
}

void spi_release(spi_t bus) {
  if (bus >= SPI_NUMOF)
    return;
  mutex_unlock(&locks[bus]);
}

uint8_t spi_transfer_byte(spi_t bus, spi_cs_t cs, bool cont, uint8_t out) {
  spi_transfer_bytes(bus, cs, cont, &out, NULL, 1);
  return 0;
}

void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont, const void *out,
                        void *in, size_t len) {
  if (bus >= SPI_NUMOF) {
    return;
  }

  // MAP_SPITransfer(GSPI_BASE, (unsigned char*)out, (unsigned char*)in, length,
  // SPI_CS_ENABLE|SPI_CS_DISABLE);
  MAP_SPITransfer(spi_config[bus].base_addr, (unsigned char *)out,
                  (unsigned char *)in, len, 0);
}

uint8_t spi_transfer_reg(spi_t bus, spi_cs_t cs, uint8_t reg, uint8_t out) {
  if (bus >= SPI_NUMOF) {
    return -1;
  }
  // MAP_SPITransfer(GSPI_BASE, &reg, 0, 1, SPI_CS_ENABLE);
  MAP_SPITransfer(spi_config[bus].base_addr, &reg, 0, 1, 0);

  // if (MAP_SPITransfer(GSPI_BASE, (unsigned char*)&out, (unsigned char*)in, 1,
  // SPI_CS_DISABLE)) {
  if (MAP_SPITransfer(spi_config[bus].base_addr, (unsigned char *)&out, 0, 1,
                      0)) {
    return -1;
  }
  return 1; // success transfer
}

void spi_transfer_regs(spi_t bus, spi_cs_t cs, uint8_t reg, const void *out,
                       void *in, size_t len) {
  if (bus >= SPI_NUMOF) {
    return;
  }
  // MAP_SPITransfer(GSPI_BASE, &reg, 0, 1, SPI_CS_ENABLE);
  MAP_SPITransfer(spi_config[bus].base_addr, &reg, 0, 1, 0);
  // if(MAP_SPITransfer(GSPI_BASE, (unsigned char*)&out, (unsigned char*)in,
  // length, SPI_CS_DISABLE)) {
  if (MAP_SPITransfer(spi_config[bus].base_addr, (unsigned char *)out,
                      (unsigned char *)in, len, 0)) {
    return;
  }
}

void spi_transmission_begin(spi_t dev, char reset_val) {
  /* spi slave is not implemented */
}
