/*
 * Copyright (C) 2015 Attilio Dona'
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     driver_periph
 * @{
 *
 * @file
 * @brief       Low-level UART driver implementation
 *
 * @author      Attilio Dona'
 *
 * @}
 */

#include <stddef.h>

#include "board.h"
#include "cpu.h"
#include "periph/uart.h"
#include "periph_conf.h"
#include "sched.h"
#include "thread.h"
#include "vendor/hw_uart.h"
#include "vendor/rom.h"
#include "xtimer.h"

#define UNUSED(x) ((x) = (x))

#define SYS_CLK 80000000

/* Bit masks for the UART Masked Interrupt Status (MIS) Register: */
#define OEMIS (1 << 10) /**< UART overrun errors */
#define BEMIS (1 << 9)  /**< UART break error */
#define FEMIS (1 << 7)  /**< UART framing error */
#define RTMIS (1 << 6)  /**< UART RX time-out */
#define RXMIS (1 << 4)  /**< UART RX masked interrupt */

// FIXME: don't know why this is needed (ROM_IntEnable should be already
// present)
#define ROM_IntEnable                                                          \
  ((void (*)(unsigned long ulInterrupt))ROM_INTERRUPTTABLE[0])

/**
 * @brief Allocate memory to store the callback functions.
 */
static uart_isr_ctx_t uart_ctx[UART_NUMOF];

static void reset(unsigned long uart_base) {
  MAP_UARTDisable(uart_base);
  MAP_UARTRxErrorClear(uart_base);
  MAP_UARTEnable(uart_base);
  MAP_UARTFIFODisable(uart_base);
}

void _uart_disable(uart_t uart) {
  volatile cc3200_uart_t *reg = uart_config[uart].dev;

  // wait for uart to finish
  while (reg->flags.bits.BUSY) {
  }

  // disable fifo
  reg->LCRH.bits.FEN = 0;

  // disable the uart
  reg->CTL.raw &= ~(UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE);
}

void _uart_enable(uart_t uart) {
  volatile cc3200_uart_t *reg = uart_config[uart].dev;

  // enable FIFO
  reg->LCRH.bits.FEN = 1;

  // enable TX, RX and UART
  reg->CTL.raw |= (UART_CTL_UARTEN | UART_CTL_TXE | UART_CTL_RXE);
}

void _uart_config(uart_t uart, uint32_t baudrate, uint32_t config) {
  // stop uart
  _uart_disable(uart);

  volatile cc3200_uart_t *reg = uart_config[uart].dev;
  uint32_t div;
  // check if baudrate is too high and needs high speed mode
  if ((baudrate * 16) > SYS_CLK) {
    reg->CTL.bits.HSE = 1;

    // half the baudrate to compensate high speed mode
    baudrate /= 2;
  } else {
    // disable high speed mode
    reg->CTL.bits.HSE = 0;
  }

  // compute & set fractional baud rate divider
  div = (((SYS_CLK * 8) / baudrate) + 1) / 2;
  reg->IBRD = div / 64;
  reg->FBRD = div % 64;

  // set config
  reg->LCRH.raw = config;

  // clear flags
  reg->flags.raw = 0;

  _uart_enable(uart);
}

void irq_handler(uart_t uart) {
  assert(uart < UART_NUMOF);

  volatile cc3200_uart_t *reg = uart_config[uart].dev;

  // get masked interrupt flags
  uint16_t mis = uart_config[uart].dev->MIS.raw;
  // clear the interrupt
  reg->ICR = mis;

  // read data
  while (uart_config[uart].dev->flags.bits.RXFE == 0) {
    uart_ctx[uart].rx_cb(uart_ctx[uart].arg, uart_config[uart].dev->dr);
  }

  if (mis & (OEMIS | BEMIS | FEMIS)) {
    // clear error status
    reg->cc3200_uart_dr.ecr = 0xFF;
  }

  cortexm_isr_end();
}

#if UART_0_ISR
void isr_uart0(void) { irq_handler((uart_t)0); }
#endif
#if UART_1_ISR
void isr_uart1(void) { irq_handler((uart_t)1); }
#endif /* UART_1_EN */

int uart_init_blocking(uart_t uart, uint32_t baudrate) {
  cc3200_periph_regs_t *periphReg;
  switch (uart) {
#if UART_0_EN
  case UART_0:
    init_periph_clk(&ARCM->UART_A0);

    ARCM->UART_A0.clk_gating |= PRCM_RUN_MODE_CLK;

    //
    // Configure PIN_55 for UART0 UART0_TX
    //
    MAP_PinTypeUART(PIN_55, PIN_MODE_3);

    //
    // Configure PIN_57 for UART0 UART0_RX
    //
    MAP_PinTypeUART(PIN_57, PIN_MODE_3);
    _uart_config(
        uart, baudrate,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    reset(UARTA0_BASE);

    break;
#endif
#if UART_1_EN
  case UART_1:
    init_periph_clk(&ARCM->UART_A1);
    ARCM->UART_A1.clk_gating |= PRCM_RUN_MODE_CLK;

    //
    // Configure PIN_07 for UART1 UART1_TX
    //
    PinTypeUART(PIN_07, PIN_MODE_5);

    //
    // Configure PIN_08 for UART1 UART1_RX
    //
    PinTypeUART(PIN_08, PIN_MODE_5);
    _uart_config(
        uart, baudrate,
        (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

    reset(UARTA1_BASE);

    break;
#endif
  default:
    return -1;
  }

  _uart_config(
      uart, baudrate,
      (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

  return 0;
}

int uart_init(uart_t uart, uint32_t baudrate, uart_rx_cb_t rx_cb, void *arg) {
  /* initialize basic functionality */
  int res = uart_init_blocking(uart, baudrate);

  /* configure interrupts and enable RX interrupt */
  switch (uart) {
#if UART_0_EN
  case UART_0:
    MAP_UARTIntEnable(UARTA0_BASE, UART_INT_RX | UART_INT_OE | UART_INT_BE |
                                       UART_INT_PE | UART_INT_FE);
    MAP_IntPrioritySet(INT_UARTA0, UART_IRQ_PRIO);
    ROM_IntEnable(INT_UARTA0);
    break;
#endif
#if UART_1_EN
  case UART_1:
    MAP_UARTIntEnable(UARTA1_BASE, UART_INT_RX | UART_INT_OE | UART_INT_BE |
                                       UART_INT_PE | UART_INT_FE);
    MAP_IntPrioritySet(INT_UARTA1, UART_IRQ_PRIO);
    ROM_IntEnable(INT_UARTA1);
    break;
#endif
  }

  return 0;
  //   volatile cc3200_uart_t *reg = uart_config[uart].dev;

  //   if (res != 0) {
  //     return res;
  //   }

  //   /* register callbacks */
  //   uart_ctx[uart].rx_cb = rx_cb;
  //   uart_ctx[uart].arg = arg;

  //   // enable uart interrupt
  //   reg->IM.raw |=
  //       UART_INT_RX | UART_INT_OE | UART_INT_BE | UART_INT_PE | UART_INT_FE;

  //   /* configure interrupts and enable RX interrupt */
  //   switch (uart) {
  // #if UART_0_EN
  //   case UART_0:
  //     ROM_IntPrioritySet(INT_UARTA0, UART_IRQ_PRIO);
  //     ROM_IntEnable(INT_UARTA0);
  //     break;
  // #endif
  // #if UART_1_EN
  //   case UART_1:
  //     MAP_IntPrioritySet(INT_UARTA1, UART_IRQ_PRIO);
  //     ROM_IntEnable(INT_UARTA1);
  //     break;
  // #endif
  //   }

  //   return 0;
}

void uart_write(uart_t uart, const uint8_t *data, size_t len) {
  volatile cc3200_uart_t *u = uart_config[uart].dev;

  /* Block if the TX FIFO is full */
  for (size_t i = 0; i < len; i++) {
    ROM_UARTCharPut((unsigned long)u, data[i]);
    // while (u->flags.bits.TXFF) {
    // }
    // u->dr = data[i];
  }
}
// TODO: enable power off and on after PWD SYS is ported
void uart_poweron(uart_t uart) { UNUSED(uart); }

void uart_poweroff(uart_t uart) { UNUSED(uart); }
