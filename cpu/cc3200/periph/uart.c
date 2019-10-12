/*
 * Copyright (C) 2019 Ludwig Maximilian Universität
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup        cpu_cc3200
 * @{
 *
 * @file
 * @brief           Driver for the cc3200 UART controller
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 *
 */
#include <stddef.h>

#include "board.h"
#include "cpu.h"
#include "periph/uart.h"
#include "periph_conf.h"
#include "sched.h"
#include "thread.h"
#include "xtimer.h"

#include "vendor/hw_uart.h"
#include "vendor/rom.h"


#define ENABLE_DEBUG (0)
#include "debug.h"


#define UART_CTL_HSE_VALUE  (0)
#define DIVFRAC_NUM_BITS    (6)
#define DIVFRAC_MASK        ((1 << DIVFRAC_NUM_BITS) - 1)

/* Bit masks for the UART Masked Interrupt Status (MIS) Register: */
#define OEMIS (1 << 10) /**< UART overrun errors */
#define BEMIS (1 << 9)  /**< UART break error */
#define FEMIS (1 << 7)  /**< UART framing error */
#define RTMIS (1 << 6)  /**< UART RX time-out */
#define RXMIS (1 << 4)  /**< UART RX masked interrupt */

/* Bit field definitions for the UART Line Control Register: */
#define FENFD (1 << 4)    /**< Enable FIFOs */

/* Valid word lengths for the LCRHbits.WLEN bit field: */
enum {
    WLEN_5_BITS = 0,
    WLEN_6_BITS = 1,
    WLEN_7_BITS = 2,
    WLEN_8_BITS = 3,
};


enum {
    FIFO_LEVEL_1_8TH = 0,
    FIFO_LEVEL_2_8TH = 1,
    FIFO_LEVEL_4_8TH = 2,
    FIFO_LEVEL_6_8TH = 3,
    FIFO_LEVEL_7_8TH = 4,
};

/* PIN_MODE value for using a PIN for UART */
#define PIN_MODE_UART 0x00000003

/* guard file in case no UART device was specified */
#if UART_NUMOF

/**
 * @brief Allocate memory to store the callback functions.
 */
static uart_isr_ctx_t uart_ctx[UART_NUMOF];

void irq_handler(uart_t uart)
{
    assert(uart < UART_NUMOF);

    volatile cc3200_uart_t *reg = uart_config[uart].dev;

    /* get masked interrupt flags */
    uint16_t mis = uart_config[uart].dev->MIS.raw;
    /* clear the interrupt */
    reg->ICR = mis;

    /* read data */
    while (uart_config[uart].dev->flags.bits.RXFE == 0) {
        uart_ctx[uart].rx_cb(uart_ctx[uart].arg, uart_config[uart].dev->dr);
    }

    if (mis & (OEMIS | BEMIS | FEMIS )) {
        /* clear error status */
        reg->cc3200_uart_dr.ecr = 0xFF;
    }

    cortexm_isr_end();
}

#if UART_0_EN
void isr_uart0(void)
{
    irq_handler((uart_t)0);
}
#endif
#if UART_1_EN
void isr_uart1(void)
{
    irq_handler((uart_t)1);
}
#endif /* UART_1_EN */

/**
 * @brief gracefully disable uart device
 *
 * @param uart interface number
 */
void uart_disable(uart_t uart)
{
    volatile cc3200_uart_t *reg = uart_config[uart].dev;

    /* wait for uart to finish */
    while (reg->flags.bits.BUSY) {
    }

    /* disable fifo */
    reg->LCRH.bits.FEN = 0;

    /* disable the uart */
    reg->CTL.raw &= ~(UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE);
}

/**
 * @brief enable uart device
 *
 * @param uart interface number∑
 */
void uart_enable(uart_t uart)
{
    volatile cc3200_uart_t *reg = uart_config[uart].dev;

    /* enable FIFO */
    reg->LCRH.bits.FEN = 1;

    /* enable TX, RX and UART */
    reg->CTL.raw |= (UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE);
}


int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg)
{
    uart_poweron(uart);

    /* Configure PIN_55 for UART0 UART0_TX */
    ROM_PinTypeUART(uart_config[uart].pin_tx, PIN_MODE_UART);

    /* Configure PIN_57 for UART0 UART0_RX */
    ROM_PinTypeUART(uart_config[uart].pin_rx, PIN_MODE_UART);

   cc3200_uart_t *u = uart_config[uart].dev;

   /* Make sure the UART is disabled before trying to configure it */
    u->CTL.raw = 0;

   /* enable UART interrupts */
    u->IM.raw = (OEMIS | BEMIS | FEMIS | RTMIS | RXMIS);

    /* Set FIFO interrupt levels and enable Rx and/or Tx: */
    if (rx_cb) {
        u->IFLS.bits.RXIFLSEL = FIFO_LEVEL_4_8TH; /**< MCU default */
        u->CTL.bits.RXE = 1;
    }
    u->IFLS.bits.TXIFLSEL = FIFO_LEVEL_4_8TH;     /**< MCU default */
    u->CTL.bits.TXE = 1;

    /* disable high speed mode */
    u->CTL.bits.HSE = 0;
    
    uint32_t div = CLOCK_CORECLOCK;
    div <<= UART_CTL_HSE_VALUE + 2;
    div += baudrate / 2; /**< Avoid a rounding error */
    div /= baudrate;
    u->IBRD = div >> DIVFRAC_NUM_BITS;
    u->FBRD = div & DIVFRAC_MASK;

    /* set config */
    u->LCRH.raw = (WLEN_8_BITS << 5) | (1 << 4);

    /* clear flags */
    u->flags.raw = 0;

    if (rx_cb) {
        uart_ctx[uart].rx_cb = rx_cb;
        uart_ctx[uart].arg   = arg;
    }

    NVIC_EnableIRQ(uart_config[uart].irqn);

    u->CTL.bits.UARTEN = 1;

    return UART_OK;
}

void uart_write(uart_t uart, const uint8_t *data, size_t len)
{
    
    cc3200_uart_t *u = uart_config[uart].dev;

    for (size_t i = 0; i < len; i++) {
        while (u->flags.bits.TXFF){}
        u->dr = data[i];
    }
}

int uart_read_blocking(uart_t uart, char *data)
{
    uint32_t u = (uint32_t)uart_config[uart].dev;
    *data      = ROM_UARTCharGet(u);

    return 1;
}

int uart_write_blocking(uart_t uart, char data)
{
    ROM_UARTCharPut((uint32_t)uart_config[uart].dev, data);
    return 1;
}

void uart_poweron(uart_t uart)
{
    switch (uart) {
#if UART_0_EN
    case UART_0:
        /* reset & enable periph clk */
        reset_periph_clk(&ARCM->UART_A0);
        ARCM->UART_A0.clk_gating |= PRCM_RUN_MODE_CLK;

        break;
#endif
#if UART_1_EN
    case UART_1:
        /* reset & enable periph clk */
        reset_periph_clk(&ARCM->UART_A1);
        ARCM->UART_A1.clk_gating |= PRCM_RUN_MODE_CLK;
        break;
#endif

    default:
        return;
    }

    uart_enable(uart);
}

void uart_poweroff(uart_t uart)
{
    /* disable uart */
    switch (uart) {
#if UART_0_EN
    case UART_0:
        ARCM->UART_A0.clk_gating &= ~PRCM_RUN_MODE_CLK;

        break;
#endif
#if UART_1_EN
    case UART_1:
        ARCM->UART_A1.clk_gating &= ~PRCM_RUN_MODE_CLK;
        break;
#endif

    default:
        return;
    }
    uart_disable(uart);
}

#endif /* UART_NUMOF */
