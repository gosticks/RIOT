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

// #include "driverlib/rom.h"
#include "vendor/hw_ints.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_timer.h"
#include "vendor/hw_types.h"
#include "vendor/rom.h"

//*****************************************************************************
//
// Values that can be passed to TimerIntEnable, TimerIntDisable, and
// TimerIntClear as the ulIntFlags parameter, and returned from TimerIntStatus.
//
//*****************************************************************************
#define TIMER_TIMB_DMA 0x00002000     // TimerB DMA Done interrupt
#define TIMER_TIMB_MATCH 0x00000800   // TimerB match interrupt
#define TIMER_CAPB_EVENT 0x00000400   // CaptureB event interrupt
#define TIMER_CAPB_MATCH 0x00000200   // CaptureB match interrupt
#define TIMER_TIMB_TIMEOUT 0x00000100 // TimerB time out interrupt
#define TIMER_TIMA_DMA 0x00000020     // TimerA DMA Done interrupt
#define TIMER_TIMA_MATCH 0x00000010   // TimerA match interrupt
#define TIMER_CAPA_EVENT 0x00000004   // CaptureA event interrupt
#define TIMER_CAPA_MATCH 0x00000002   // CaptureA match interrupt
#define TIMER_TIMA_TIMEOUT 0x00000001 // TimerA time out interrupt

//*****************************************************************************
//
// Values that can be passed to most of the timer APIs as the ulTimer
// parameter.
//
//*****************************************************************************
#define TIMER_A 0x000000ff    // Timer A
#define TIMER_B 0x0000ff00    // Timer B
#define TIMER_BOTH 0x0000ffff // Timer Both

//*****************************************************************************
//
// Values that can be passed to TimerConfigure as the ulConfig parameter.
//
//*****************************************************************************

#define TIMER_CFG_ONE_SHOT 0x00000021 // Full-width one-shot timer
#define TIMER_CFG_ONE_SHOT_UP                                                  \
  0x00000031                          // Full-width one-shot up-count
                                      // timer
#define TIMER_CFG_PERIODIC 0x00000022 // Full-width periodic timer
#define TIMER_CFG_PERIODIC_UP                                                  \
  0x00000032                            // Full-width periodic up-count
                                        // timer
#define TIMER_CFG_SPLIT_PAIR 0x04000000 // Two half-width timers

#define TIMER_CFG_A_ONE_SHOT 0x00000021     // Timer A one-shot timer
#define TIMER_CFG_A_ONE_SHOT_UP 0x00000031  // Timer A one-shot up-count timer
#define TIMER_CFG_A_PERIODIC 0x00000022     // Timer A periodic timer
#define TIMER_CFG_A_PERIODIC_UP 0x00000032  // Timer A periodic up-count timer
#define TIMER_CFG_A_CAP_COUNT 0x00000003    // Timer A event counter
#define TIMER_CFG_A_CAP_COUNT_UP 0x00000013 // Timer A event up-counter
#define TIMER_CFG_A_CAP_TIME 0x00000007     // Timer A event timer
#define TIMER_CFG_A_CAP_TIME_UP 0x00000017  // Timer A event up-count timer
#define TIMER_CFG_A_PWM 0x0000000A          // Timer A PWM output
#define TIMER_CFG_B_ONE_SHOT 0x00002100     // Timer B one-shot timer
#define TIMER_CFG_B_ONE_SHOT_UP 0x00003100  // Timer B one-shot up-count timer
#define TIMER_CFG_B_PERIODIC 0x00002200     // Timer B periodic timer
#define TIMER_CFG_B_PERIODIC_UP 0x00003200  // Timer B periodic up-count timer
#define TIMER_CFG_B_CAP_COUNT 0x00000300    // Timer B event counter
#define TIMER_CFG_B_CAP_COUNT_UP 0x00001300 // Timer B event up-counter
#define TIMER_CFG_B_CAP_TIME 0x00000700     // Timer B event timer
#define TIMER_CFG_B_CAP_TIME_UP 0x00001700  // Timer B event up-count timer
#define TIMER_CFG_B_PWM 0x00000A00          // Timer B PWM output

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

#define ROM_TimerConfigure                                                     \
  ((void (*)(unsigned long ulBase, unsigned long ulConfig))ROM_TIMERTABLE[2])

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

  // enable & reset periph clock
  cc3200_periph_regs_t *periphReg =
      ((cc3200_periph_regs_t **)ARCM)[timer_config[dev].periph_id];
  init_periph_clk(periphReg);

  // setup timer
  ROM_TimerConfigure(base, TIMER_CFG_PERIODIC_UP);
  ROM_TimerControlStall(base, TIMER_A, true);

  // register & setup intrrupt handling
  ROM_TimerIntRegister(base, TIMER_A, timerHandler);
  timer_config[dev].ctx.cb = cb;
  ROM_IntPrioritySet(timer_config[dev].irqn, INT_PRIORITY_LVL_2);

  // enable the timer
  ROM_TimerEnable(base, TIMER_A);

  return 0;
}

int set_absolute(tim_t dev, int channel, unsigned long long value) {
  if (dev >= MAX_TIMERS) {
    return -1;
  }
  ROM_TimerMatchSet((uint32_t)timer_config[dev].timer, TIMER_A, value);
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
  ROM_TimerIntClear((uint32_t)timer_config[dev].timer, TIMER_TIMA_MATCH);
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
  ROM_TimerEnable((uint32_t)timer_config[dev].timer, TIMER_A);
}

void timer_stop(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return;
  }
  ROM_TimerDisable((uint32_t)timer_config[dev].timer, TIMER_A);
}

void timer_irq_enable(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return;
  }
  ROM_TimerIntEnable((uint32_t)timer_config[dev].timer, TIMER_TIMA_MATCH);
}

void timer_irq_disable(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return;
  }
  ROM_TimerIntDisable((uint32_t)timer_config[dev].timer, TIMER_TIMA_MATCH);
}

void timer_reset(tim_t dev) {
  if (dev >= MAX_TIMERS) {
    return;
  }

  // TODO: for now only timer a is supported
  timer_config[dev].timer->val_a = 0;
}
