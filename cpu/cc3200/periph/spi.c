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

#include "cpu.h"
#include "mutex.h"
#include "periph/gpio.h"
#include "periph/spi.h"
#include "periph_conf.h"
#include "board.h"

#include "vendor/hw_udma.h"
#include "vendor/hw_ints.h"
#include "driverlib/prcm.h"
#include "driverlib/pin.h"
#include "driverlib/spi.h"
#include "driverlib/utils.h"

#define ENABLE_DEBUG (0)
#include "debug.h"
/**
 * @brief   Default SPI device access macro
 */
#ifndef SPI_DEV
#define SPI_DEV(x) (x)
#endif

#define SPI_NUMOF 2U
#define EXTERNAL_SPI 0U
#define CC3100_SPI 1U

/* guard this file in case no SPI device is defined */
// #if SPI_NUMOF

static unsigned long bitrate[] = {
    [SPI_CLK_100KHZ] = 100000,
    [SPI_CLK_400KHZ] = 400000,
    [SPI_CLK_1MHZ] = 1000000,
    [SPI_CLK_5MHZ] = 5000000,
    [SPI_CLK_10MHZ] = 10000000};

static const spi_conf_t spi_config[] = {
    {
        .busaddr = GSPI_BASE,
        .sclk    = PIN_05,
        .miso    = PIN_06,
        .mosi    = PIN_07,
        .cs      = PIN_08
    },
    {
        .busaddr = LSPI_BASE,
    }
};


/**
 * @brief   Allocate one lock per SPI device
 */
static mutex_t locks[SPI_NUMOF];

void spi_init_pins(spi_t bus) {
    
    switch(bus) {
    case EXTERNAL_SPI:
        // enable peripherial clock
        MAP_PRCMPeripheralClkEnable(PRCM_GSPI, PRCM_RUN_MODE_CLK);
        // TODO: use gpio_init for this when PIN_MODE mapping is done
        // GSPI_CLK
        MAP_PinTypeSPI(spi_config[bus].sclk, PIN_MODE_7);
        // set MISO pin
        MAP_PinTypeSPI(spi_config[bus].miso, PIN_MODE_7);
        // set GSPI_MOSI
        MAP_PinTypeSPI(spi_config[bus].mose, PIN_MODE_7);
        // set GSPI_CS
        MAP_PinTypeSPI(spi_config[bus].cs, PIN_MODE_7);

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
    //Disable Chip Select
    MAP_SPICSDisable(spi_config[bus].busaddr);

    //Disable SPI Channel
    MAP_SPIDisable(spi_config[bus].busaddr);

    // reset SPI 
    MAP_SPIReset(spi_config[bus].busaddr);

    MAP_SPIEnable(spi_config[bus].busaddr);
}

void spi_init(spi_t bus) {
    assert(bus < SPI_NUMOF);

    mutex_init(&locks[bus]);
    // trigger pin initialization
    spi_init_pins(bus);

    // enable clock
    switch(bus) {
        case EXTERNAL_SPI:
            enable_peiph_clk(PRCM_GSPI, PRCM_RUN_MODE_CLK);
            break;
        case CC3100_SPI:
            enable_peiph_clk(PRCM_LSPI,PRCM_RUN_MODE_CLK|PRCM_SLP_MODE_CLK);
            break;
    }

    // reset spi for the changes to take effect
    spi_reset(bus);
}


int spi_acquire(spi_t bus, spi_cs_t cs, spi_mode_t mode, spi_clk_t clk)
{
    /* lock bus */
    mutex_lock(&locks[bus]);

    if (bus >= SPI_UNDEF) return -1;

    // TODO: use cs mode and clock for now only master is supported with the default clocks
    // enable clock
    switch(bus) {
        case EXTERNAL_SPI:
            MAP_SPIConfigSetExpClk(
                spi_config[bus].busaddr, 
                MAP_PRCMPeripheralClockGet(PRCM_GSPI),
                20000000, 
                SPI_MODE_MASTER, 
                conf,
                (SPI_HW_CTRL_CS |
                SPI_4PIN_MODE |
                SPI_TURBO_OFF |
                SPI_CS_ACTIVELOW |
                SPI_WL_8));
            break;
        case CC3100_SPI:    
            MAP_SPIConfigSetExpClk(
                spi_config[bus].busaddr,
                MAP_PRCMPeripheralClockGet(PRCM_LSPI),
                20000000,
                SPI_MODE_MASTER,
                SPI_SUB_MODE_0,
                (SPI_SW_CTRL_CS |
                    SPI_4PIN_MODE |
                    SPI_TURBO_OFF |
                    SPI_CS_ACTIVEHIGH |
                    SPI_WL_32
                )
            );
            break;
    }
    return 0;
}

void spi_release(spi_t bus)
{
    if (bus >= SPI_NUMOF) return;
    mutex_unlock(&locks);
}


uint8_t spi_transfer_byte(spi_t bus, spi_cs_t cs, bool cont, uint8_t out)
{
    spi_transfer_bytes(bus, cs, cont, &out, NULL, 1);
    return 0;
}

void spi_transfer_bytes(spi_t bus, spi_cs_t cs, bool cont, const void *out, void *in, size_t len)
{
    UNUSED(cs);
    UNUSED(cont);
    if (bus >= SPI_NUMOF)
    {
        return;
    }

    //MAP_SPITransfer(GSPI_BASE, (unsigned char*)out, (unsigned char*)in, length, SPI_CS_ENABLE|SPI_CS_DISABLE);
    MAP_SPITransfer(spi_config[bus].baseaddr, (unsigned char *)out, (unsigned char *)in, len, 0);
}

uint8_t spi_transfer_reg(spi_t bus, spi_cs_t cs, uint8_t reg, uint8_t out)
{
    UNUSED(cs);
    if (bus >= SPI_NUMOF)
    {
        return -1;
    }
    //MAP_SPITransfer(GSPI_BASE, &reg, 0, 1, SPI_CS_ENABLE);
    MAP_SPITransfer(spi_config[bus].baseaddr, &reg, 0, 1, 0);

    //if (MAP_SPITransfer(GSPI_BASE, (unsigned char*)&out, (unsigned char*)in, 1, SPI_CS_DISABLE)) {
    if (MAP_SPITransfer(spi_config[bus].baseaddr, (unsigned char *)&out, 0, 1, 0))
    {
        return -1;
    }
    return 1; // success transfer
}

void spi_transfer_regs(spi_t bus, spi_cs_t cs, uint8_t reg, const void *out, void *in, size_t len)
{
    UNUSED(cs);
    if (bus >= SPI_NUMOF)
    {
        return;
    }
    //MAP_SPITransfer(GSPI_BASE, &reg, 0, 1, SPI_CS_ENABLE);
    MAP_SPITransfer(spi_config[bus].baseaddr, &reg, 0, 1, 0);
    //if(MAP_SPITransfer(GSPI_BASE, (unsigned char*)&out, (unsigned char*)in, length, SPI_CS_DISABLE)) {
    if (MAP_SPITransfer(spi_config[bus].baseaddr, (unsigned char *)out, (unsigned char *)in, len, 0))
    {
        return;
    }
}

void spi_transmission_begin(spi_t dev, char reset_val)
{

    UNUSED(dev);
    UNUSED(reset_val);
    /* spi slave is not implemented */
}
