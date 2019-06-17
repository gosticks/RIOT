/*
 * Copyright (C) 2019 Ludwig Maximilian Universität
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup        cpu_cc3200
 * @{
 *
 * @file
 * @brief           Timer implementation
 * handling
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 */
#include <stdlib.h>

#include "panic.h"
#include <sys/types.h>
#include <thread.h>

#include "periph/timer.h"
#include "periph_conf.h"
#include "xtimer.h"

#include "driverlib/prcm.h"
#include "driverlib/timer.h"

#include "vendor/hw_ints.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_timer.h"
#include "vendor/hw_types.h"

#define MAX_TIMERS TIMER_NUMOF

typedef struct {
  uint32_t conf;             // configuration
  uint32_t timer_a_mode;     // timer A Mode
  uint32_t timer_b_mode;     // timer B Mode
  uint32_t ctrl;             // timer control register
  uint8_t sync[8];           // sync
  uint32_t intr_mask;        // interrupt mask
  uint32_t intr_raw_stat;    // raw interrupt status
  uint32_t masked_intr;      // masked interrupt
  uint32_t intr_clear;       // interrupt clear
  uint32_t interval_load_a;  // interval load a
  uint32_t interval_load_b;  // interval load b
  uint32_t match_a;          // timer match a
  uint32_t match_b;          // timer match b
  uint32_t prescale_a;       // timer prescale a
  uint32_t prescale_b;       // timer prescale b
  uint32_t prescale_match_a; // timer prescale match a
  uint32_t prescale_match_b; // timer prescale match b
  uint32_t timer_a;          // timer a
  uint32_t timer_b;          // timer b
  uint32_t val_a;            // timer value a
  uint32_t val_b;            // timer value b
  uint32_t rtc_predivide;    // RTC Predivide

  // snapshots
  uint32_t prescale_snaphot_a;  // timer prescale snapshot a
  uint32_t prescale_snapshot_b; // timer prescale snapshot b
  uint32_t val_snapshot_a;      // timer value snapshot a
  uint32_t val_snapshot_b;      // timer value snapshot b
  uint32_t dma_event;           // DMA event
} cc3200_timer;

typedef struct {
  cc3200_timer *timer; // RIOT specific timer ID
  bool init;           // indicates if timer is ready
  int periph_id;       // TI id for the periph
  uint8_t irqn;
  timer_isr_ctx_t ctx;
} timer_conf_t;

static timer_conf_t timer_config[] = {{
                                          .timer = (cc3200_timer *)TIMERA0_BASE,
                                          .irqn = INT_TIMERA0A,
                                          .periph_id = PRCM_TIMERA0,
                                      },
                                      {
                                          .timer = (cc3200_timer *)TIMERA1_BASE,
                                          .irqn = INT_TIMERA1A,
                                          .periph_id = PRCM_TIMERA1,
                                      },
                                      {
                                          .timer = (cc3200_timer *)TIMERA2_BASE,
                                          .irqn = INT_TIMERA2A,
                                          .periph_id = PRCM_TIMERA2,

                                      },
                                      {
                                          .timer = (cc3200_timer *)TIMERA3_BASE,
                                          .irqn = INT_TIMERA3A,
                                          .periph_id = PRCM_TIMERA3,
                                      }};

void timerHandler(tim_t dev) {
  timer_clear(dev, 0);
  timer_config[dev].ctx.cb(timer_config[dev].ctx.arg,
                           0); // timer has one hw channel
  cortexm_isr_end();
}

void irqTimer0Handler(void) { timerHandler(T0); }
#ifdef T1
void irqTimer1Handler(void) { timerHandler(T1); }
#endif
#ifdef T2
void irqTimer2Handler(void) { timerHandler(T2); }
#endif
#ifdef T3
void irqTimer3Handler(void) { timerHandler(T3); }
#endif

static inline void *getHandler(tim_t dev) {
  switch (dev) {
#ifdef T1
  case T1:
    return irqTimer1Handler;
#endif
#ifdef T2
  case T2:
    return irqTimer2Handler;
#endif
#ifdef T3
  case T3:
    return irqTimer3Handler;
#endif
  default:
    return irqTimer0Handler;
  };
}

int timer_init(tim_t dev, unsigned long freq, timer_cb_t cb, void *arg) {

  // check if timer id is valid
  if (dev >= MAX_TIMERS) {
    return -1;
  }
  void *timerHandler = getHandler(dev);
  if (timerHandler == NULL) {
    return -1;
  }

  uint32_t base = (int)timer_config[dev].timer;

  // enable periph clock
  MAP_PRCMPeripheralClkEnable(timer_config[dev].periph_id, PRCM_RUN_MODE_CLK);

  // reset timer
  MAP_PRCMPeripheralReset(timer_config[dev].periph_id);

  // setup timer
  MAP_TimerConfigure(base, TIMER_CFG_PERIODIC_UP);
  MAP_TimerControlStall(base, TIMER_A, true);

  // register & setup intrrupt handling
  MAP_TimerIntRegister(base, TIMER_A, timerHandler);
  timer_config[dev].ctx.cb = cb;
  MAP_IntPrioritySet(timer_config[dev].irqn, INT_PRIORITY_LVL_2);

  // enable the timer
  MAP_TimerEnable(base, TIMER_A);

  return 0;
}

int set_absolute(tim_t dev, int channel, unsigned long long value) {
  if (dev >= MAX_TIMERS) {
    return -1;
  }
  MAP_TimerMatchSet((uint32_t)timer_config[dev].timer, TIMER_A, value);
  timer_config[dev].timer->intr_mask |= TIMER_TIMA_MATCH;
  return 0;
}

int timer_set(tim_t dev, int channel, unsigned int timeout) {
  return set_absolute(dev, channel, timer_config[dev].timer->timer_a + timeout);
}

int timer_set_absolute(tim_t dev, int channel, unsigned int value) {
  return set_absolute(dev, channel, value);
}

int timer_clear(tim_t dev, int channel) {
  if (dev >= MAX_TIMERS) {
    return -1;
  }
  MAP_TimerIntClear((uint32_t)timer_config[dev].timer, TIMER_TIMA_MATCH);
  // disable the match timer
  timer_config[dev].timer->intr_mask &= ~(TIMER_TIMA_MATCH);
  return 0;
}

unsigned int timer_read(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return 0;
  }
  return timer_config[dev].timer->val_a;
}

void timer_start(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return;
  }
  MAP_TimerEnable((uint32_t)timer_config[dev].timer, TIMER_A);
}

void timer_stop(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return;
  }
  MAP_TimerDisable((uint32_t)timer_config[dev].timer, TIMER_A);
}

void timer_irq_enable(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return;
  }
  MAP_TimerIntEnable((uint32_t)timer_config[dev].timer, TIMER_TIMA_MATCH);
}

void timer_irq_disable(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return;
  }
  MAP_TimerIntDisable((uint32_t)timer_config[dev].timer, TIMER_TIMA_MATCH);
}

void timer_reset(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return;
  }

  // TODO: for now only timer a is supported
  timer_config[dev].timer->val_a = 0;
}
