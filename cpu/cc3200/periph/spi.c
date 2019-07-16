/*
 * Copyright (C) 2019 Ludwig Maximilian Universität
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup        cpu_cc3200_spi CC3200 SPI
 * @ingroup         cpu_cc3200_regs
 * @{
 *
 * @file
 * @brief           Driver for the cc3200 SPI controller
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

// #define SPI_NUMOF 2U
#define EXTERNAL_SPI 0U
#define CC3100_SPI 1U
#define XTAL_CLK 40000000
#define PIN_MODE_SPI 0x00000007

/**
 * @brief   Allocate one lock per SPI device
 */
static mutex_t locks[SPI_NUMOF];

/**
 * @brief init pins for a bus
 *
 * @param bus
 */
void spi_init_pins(spi_t bus)
{
	// check if sck and mosi pins are zero indicating no pin setup
	if (spi_config[bus].pins.sck == 0 && spi_config[bus].pins.mosi == 0) {
		return;
	}

	// GSPI_CLK
	ROM_PinTypeSPI(spi_config[bus].pins.sck, PIN_MODE_SPI);
	// set MISO pin
	ROM_PinTypeSPI(spi_config[bus].pins.miso, PIN_MODE_SPI);
	// set GSPI_MOSI
	ROM_PinTypeSPI(spi_config[bus].pins.mosi, PIN_MODE_SPI);
	// set GSPI_CS
	ROM_PinTypeSPI(spi_config[bus].pins.cs, PIN_MODE_SPI);
}

/**
 * @brief utility returning spi struct for a bus id
 *
 * @param bus
 * @return cc3200_spi_t*
 */
static inline cc3200_spi_t *spi(spi_t bus)
{
	return (cc3200_spi_t *)spi_config[bus].base_addr;
}

/**
 * @brief reset spi to default state
 *
 * @param bus spi bus id
 */
void spi_reset(spi_t bus)
{
	volatile cc3200_spi_t *dev = spi(bus);

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
void _spi_config(spi_t bus, spi_mode_t mode, spi_clk_t clk)
{
	volatile cc3200_spi_t *dev = spi(bus);
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
	tmp = (tmp &
	       ~(MCSPI_CH0CONF_WL_M | MCSPI_CH0CONF_EPOL | MCSPI_CH0CONF_POL |
		 MCSPI_CH0CONF_PHA | MCSPI_CH0CONF_TURBO)) |
	      MCSPI_CH0CONF_CLKG;

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

void spi_init(spi_t bus)
{
	// assert(bus >= SPI_NUMOF);
	mutex_init(&locks[bus]);
	// trigger pin initialization
	spi_init_pins(bus);

	// enable clock
	switch (bus) {
	case EXTERNAL_SPI:
		ARCM->MCSPI_A1.clk_gating |= PRCM_RUN_MODE_CLK;
		break;
	case CC3100_SPI:
		ARCM->MCSPI_A2.clk_gating |=
			PRCM_RUN_MODE_CLK | PRCM_SLP_MODE_CLK;
		break;
	}

	// reset spi for the changes to take effect
	spi_reset(bus);
}

int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk)
{
	if (bus >= SPI_UNDEF) {
		return -1;
	}

	// lock bus
	mutex_lock(&locks[bus]);

	if (cs == GPIO_UNDEF) {
		spi(bus)->ch0_conf &= ~MCSPI_CH0CONF_FORCE;
	} else {
		spi(bus)->ch0_conf |= MCSPI_CH0CONF_FORCE;
	}

	_spi_config(bus, mode, clk);

	spi(bus)->ch0_ctrl |= MCSPI_CH0CTRL_EN;
	return 0;
}

void spi_release(spi_t bus)
{
	if (bus >= SPI_NUMOF)
		return;
	mutex_unlock(&locks[bus]);

	// disable spi
	spi(bus)->ch0_ctrl &= ~MCSPI_CH0CTRL_EN;
}

uint8_t spi_transfer_byte(spi_t bus, spi_cs_t cs, bool cont, uint8_t out)
{
	spi_transfer_bytes(bus, cs, cont, &out, NULL, 1);
	return 0;
}

void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont, const void *out,
			void *in, size_t len)
{
	// disable or enable cs
	if (cs == GPIO_UNDEF) {
		spi(bus)->ch0_conf &= ~MCSPI_CH0CONF_FORCE;
	} else {
		spi(bus)->ch0_conf |= MCSPI_CH0CONF_FORCE;
	}
	ROM_SPITransfer((unsigned long)spi(bus), (unsigned char *)out,
			(unsigned char *)in, len, 0x00000001);

	// if cs was enabled disable it again
	if (cs != GPIO_UNDEF) {
		spi(bus)->ch0_conf &= ~MCSPI_CH0CONF_FORCE;
	}
}

uint8_t spi_transfer_reg(spi_t bus, spi_cs_t cs, uint8_t reg, uint8_t out)
{
	if (bus >= SPI_NUMOF) {
		return -1;
	}
	ROM_SPITransfer(spi_config[bus].base_addr, &reg, 0, 1, 0);

	if (ROM_SPITransfer(spi_config[bus].base_addr, (unsigned char *)&out, 0,
			    1, 0)) {
		return -1;
	}
	return 1; // success transfer
}

void spi_transfer_regs(spi_t bus, spi_cs_t cs, uint8_t reg, const void *out,
		       void *in, size_t len)
{
	if (bus >= SPI_NUMOF) {
		return;
	}
	ROM_SPITransfer(spi_config[bus].base_addr, &reg, 0, 1, 0);
	if (ROM_SPITransfer(spi_config[bus].base_addr, (unsigned char *)out,
			    (unsigned char *)in, len, 0)) {
		return;
	}
}
