/*
 * Copyright (C) 2019 Ludwig Maximilian Universität
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
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 *
 * @{
 */
#include "periph/spi.h"
#include "board.h"
#include "cpu.h"
#include "mutex.h"
#include "periph/gpio.h"
#include "periph_conf.h"

#include "vendor/hw_ints.h"
#include "vendor/hw_mcspi.h"
#include "vendor/hw_udma.h"
#include "vendor/rom.h"
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
#define XTAL_CLK 40000000

static const spi_conf_t spi_config[] = {
    {.base_addr = GSPI_BASE,
     .gpio_port = 0,
     .pins =
         (spi_pins_t){
             .miso = PIN_06, .sck = PIN_05, .mosi = PIN_07, .cs = PIN_08},
     .config = (SPI_HW_CTRL_CS | SPI_4PIN_MODE | SPI_TURBO_OFF |
                SPI_CS_ACTIVELOW | SPI_WL_8)},
    {.base_addr = LSPI_BASE,
     .gpio_port = 1,
     .config = (SPI_SW_CTRL_CS | SPI_4PIN_MODE | SPI_TURBO_OFF |
                SPI_CS_ACTIVEHIGH | SPI_WL_32)}};

/**
 * @brief   Allocate one lock per SPI device
 */
static mutex_t locks[SPI_NUMOF];

void spi_init_pins(spi_t bus) {

  switch (bus) {
  case EXTERNAL_SPI:
    // GSPI_CLK
    ROM_PinTypeSPI(spi_config[bus].pins.sck, PIN_MODE_7);
    // set MISO pin
    ROM_PinTypeSPI(spi_config[bus].pins.miso, PIN_MODE_7);
    // set GSPI_MOSI
    ROM_PinTypeSPI(spi_config[bus].pins.mosi, PIN_MODE_7);
    // set GSPI_CS
    ROM_PinTypeSPI(spi_config[bus].pins.cs, PIN_MODE_7);

    break;
  case CC3100_SPI:
    // no setup is required for the wifi spi since the pins cannot be reset
    // (except for UART but that is not supported by design)
    break;
  }
}

static inline cc3200_spi_t *spi(uint32_t baseAddr) {
  return (cc3200_spi_t *)baseAddr;
}

static inline uint32_t get_word_len(uint32_t conf) {
  // use WL_32 as mask
  return conf | SPI_WL_32;
}

/**
 * @brief reset spi to default state
 *
 * @param bus spi bus id
 */
void spi_reset(spi_t bus) {
  volatile cc3200_spi_t *dev = spi(spi_config[bus].base_addr);

  // disable chip select in software controlled mode
  dev->ch0_conf &= ~MCSPI_CH0CONF_FORCE;

  // Disable SPI Channel
  dev->ch0_ctrl &= ~MCSPI_CH0CTRL_EN;

  // reset SPI
  dev->sys_conf |= MCSPI_SYSCONFIG_SOFTRESET;

  // wait for reset
  while (!((dev->sys_status) & MCSPI_SYSSTATUS_RESETDONE)) {
  }

  // enable spi
  dev->ch0_ctrl &= ~MCSPI_CH0CTRL_EN;
}

/**
 * @brief configure spi module. This functions is closely modeled after
 * SPIConfigSetExpClk.
 *
 * @param bus
 * @param mode SPI operation sub mode
 * @param clk SPI bit rate
 */
void _spi_config(spi_t bus, spi_mode_t mode, spi_clk_t clk) {
  volatile cc3200_spi_t *dev = spi(spi_config[bus].base_addr);
  // current value of the ctrl register is used as a starting point
  cc3200_reg_t tmp = dev->module_ctrl;

  // compute divider value
  cc3200_reg_t divider = (XTAL_CLK / clk) - 1;

  // set master mode with hardware chip select
  tmp &= ~(MCSPI_MODULCTRL_MS | MCSPI_MODULCTRL_SINGLE);

  // set SPI config
  // TIs code is also using OR with Master/Slave mode
  // since riot only supports master mode we can skip that
  tmp |= (spi_config[bus].config >> 24) & 0xFF;

  // write config
  dev->module_ctrl = tmp;

  // reset tmp and set IS, DPE0, DPE1 for master mode
  tmp = 0x1 << 16;

  // mask config and set clock divider granularity to 1 cycle
  tmp &= ~(MCSPI_CH0CONF_WL_M | MCSPI_CH0CONF_EPOL | MCSPI_CH0CONF_POL |
           MCSPI_CH0CONF_PHA | MCSPI_CH0CONF_TURBO);
  tmp |= MCSPI_CH0CONF_CLKG;

  //
  // The least significant four bits of the divider is used fo configure
  // CLKD in MCSPI_CHCONF next eight least significant bits are used to
  // configure the EXTCLK in MCSPI_CHCTRL
  //
  tmp |= ((divider & 0x0000000F) << 2);
  dev->ch0_ctrl = ((divider & 0x00000FF0) << 4);

  // set protocol, CS polarity, word length and turbo mode
  dev->ch0_conf = ((tmp | mode) | (spi_config[bus].config & 0x0008FFFF));
}

void spi_init(spi_t bus) {
  assert(bus < SPI_NUMOF);

  mutex_init(&locks[bus]);
  // trigger pin initialization
  spi_init_pins(bus);

  // enable clock
  switch (bus) {
  case EXTERNAL_SPI:
    ARCM->MCSPI_A1.clk_gating |= PRCM_RUN_MODE_CLK;
    break;
  case CC3100_SPI:
    ARCM->MCSPI_A2.clk_gating |= PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK;
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

  _spi_config(bus, mode, clk);

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
  DEBUG("%s: bus=%u, len=%u\n", __FUNCTION__, bus, (unsigned)len);
  ROM_SPITransfer(spi_config[bus].base_addr, (unsigned char *)out,
                  (unsigned char *)in, len, 0);
  // // check that at least read or write buffer is set
  // assert(in || out);

  // // initialize icrements for read and write buffers
  // size_t writeIncr = out == NULL ? 0 : 1;
  // size_t readIncr = in == NULL ? 0 : 1;
  // // check if data size is compatible with SPI word length and adjust length
  // switch (get_word_len(spi_config[bus].config)) {
  // case SPI_WL_32:
  //   assert(!(len % 4));
  //   writeIncr *= 4;
  //   readIncr *= 4;
  //   break;
  // case SPI_WL_16:
  //   assert(!(len % 2));
  //   writeIncr *= 2;
  //   readIncr *= 2;
  //   break;
  // }

  // cc3200_spi_t *dev = spi(spi_config[bus].base_addr);

  // // if (cs != SPI_CS_UNDEF) {
  // //   gpio_clear((gpio_t)cs);
  // // }

  // // enable cs flags
  // // dev->ch0_conf |= MCSPI_CH0CONF_FORCE;

  // while (len) {
  //   // send one word of data
  //   while (!(dev->ch0_stat & MCSPI_CH0STAT_TXS)) {
  //   }
  //   dev->tx0 = *((uint32_t *)out);

  //   // read one word of response
  //   while (!(dev->ch0_stat & MCSPI_CH0STAT_RXS)) {
  //   }
  //   *((uint32_t *)in) = dev->rx0;

  //   // increment pointers
  //   out += writeIncr;
  //   in += readIncr;
  // }

  // ROM_SPITransfer(GSPI_BASE, (unsigned char*)out, (unsigned char*)in,
  // length, SPI_CS_ENABLE|SPI_CS_DISABLE);
  // ROM_SPITransfer(spi_config[bus].base_addr, (unsigned char *)out,
  //                 (unsigned char *)in, len, 0);
}

uint8_t spi_transfer_reg(spi_t bus, spi_cs_t cs, uint8_t reg, uint8_t out) {
  if (bus >= SPI_NUMOF) {
    return -1;
  }
  // ROM_SPITransfer(GSPI_BASE, &reg, 0, 1, SPI_CS_ENABLE);
  ROM_SPITransfer(spi_config[bus].base_addr, &reg, 0, 1, 0);

  // if (ROM_SPITransfer(GSPI_BASE, (unsigned char*)&out, (unsigned char*)in,
  // 1, SPI_CS_DISABLE)) {
  if (ROM_SPITransfer(spi_config[bus].base_addr, (unsigned char *)&out, 0, 1,
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
  // ROM_SPITransfer(GSPI_BASE, &reg, 0, 1, SPI_CS_ENABLE);
  ROM_SPITransfer(spi_config[bus].base_addr, &reg, 0, 1, 0);
  // if(ROM_SPITransfer(GSPI_BASE, (unsigned char*)&out, (unsigned char*)in,
  // length, SPI_CS_DISABLE)) {
  if (ROM_SPITransfer(spi_config[bus].base_addr, (unsigned char *)out,
                      (unsigned char *)in, len, 0)) {
    return;
  }
}
