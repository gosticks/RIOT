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

#include "vendor/hw_ints.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_timer.h"
#include "vendor/hw_types.h"
#include "vendor/rom.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

#define LOAD_VALUE (0xffff)

/**
 * @brief Timer state memory
 */
static timer_isr_ctx_t isr_ctx[TIMER_NUMOF];

/**
 * @brief get timer control register by ID. Each register is 0x1000 apart
 * starting at 0x40030000
 *
 *
 */
static inline cc3200_timer_t *timer(tim_t num)
{
	return (cc3200_timer_t *)(0x40030000 + (num << 12));
}

/**
 * @brief returns the timer periphirial register used to enable or disable
 * hardware periphiria.
 *
 */
static inline cc3200_periph_regs_t *timer_periph_reg(tim_t num)
{
	return (cc3200_periph_regs_t *)((&ARCM->GPT_A0) +
					sizeof(cc3200_periph_regs_t) * num);
}

/**
 * @brief timer interrupt handler
 *
 * @param[in] dev GPT instance number
 */
static void timer_irq_handler(tim_t dev)
{
	timer_clear(dev, 0);
	isr_ctx[dev].cb(isr_ctx[dev].arg,
			0); // timer has one hw channel
	cortexm_isr_end();
}
void isr_timer0(void)
{
	timer_irq_handler(TIMER_0_EN);
}

#ifdef TIMER_1_EN
void isr_timer1(void)
{
	timer_irq_handler(TIMER_1_EN);
}
#endif

#ifdef TIMER_2_EN
void isr_timer2(void)
{
	timer_irq_handler(TIMER_2_EN);
}
#endif

#ifdef TIMER_3_EN
void isr_timer3(void)
{
	timer_irq_handler(TIMER_3_EN);
}
#endif

/**
 * @brief Get the irq handler object
 *
 * @param dev
 * @return void*
 */
static inline void *get_irq_handler(tim_t dev)
{
	switch (dev) {
	case TIMER_0_EN:
		return isr_timer0;
#ifdef TIMER_1_EN
	case TIMER_1_EN:
		return isr_timer1;
#endif
#ifdef TIMER_2_EN
	case TIMER_2_EN:
		return isr_timer2;
#endif
#ifdef TIMER_3_EN
	case TIMER_3_EN:
		return isr_timer3;
#endif
	default:
		// requested irq handler for invalid timer
		DEBUG("REQUESTED IRQ FOR INVALID TIMER");
		assert(0);
	};
}

#define ROM_TimerConfigure               \
	((void (*)(unsigned long ulBase, \
		   unsigned long ulConfig))ROM_TIMERTABLE[2])

int timer_init(tim_t dev, unsigned long freq, timer_cb_t cb, void *arg)
{
	// check if timer id is valid
	if (dev >= MAX_TIMERS) {
		return -1;
	}
	void *timerHandler = get_irq_handler(dev);
	uint32_t prescaler = 0;
	if (timerHandler == NULL) {
		return -1;
	}

	// get periph register by adding dev * sizeof(periph_reg) to the
	// first timer periph register
	cc3200_periph_regs_t *periphReg = timer_periph_reg(dev);

	// enable & reset periph clock
	periphReg->clk_gating |= PRCM_RUN_MODE_CLK;
	reset_periph_clk(periphReg);

	// setup timer (currently only a timer is supported)
	ROM_TimerConfigure((uint32_t)timer(dev), TIMER_CFG_A_PERIODIC_UP);
	ROM_TimerControlStall((uint32_t)timer(dev), TIMER_A, true);

	// set prescale
	// only 16bit timer mode is currently supported
	// if 32bit mode is enabled freq can only be the same as system freq
	prescaler = CLOCK_CORECLOCK;
	prescaler += freq / 2;
	prescaler /= freq;
	if (prescaler > 0) {
		prescaler--;
	}
	if (prescaler > 255) {
		prescaler = 255;
	}

	timer(dev)->prescale_a      = prescaler;
	timer(dev)->interval_load_a = LOAD_VALUE;

	// register & setup intrrupt handling
	ROM_TimerIntRegister((uint32_t)timer(dev), TIMER_A, timerHandler);
	isr_ctx[dev].cb  = cb;
	isr_ctx[dev].arg = arg;

	// timer A irqn (B now supported) is always two apart
	ROM_IntPrioritySet(INT_TIMERA0A + dev * 2, INT_PRIORITY_LVL_2);

	// enable the timer
	ROM_TimerEnable((uint32_t)timer(dev), TIMER_A);

	return 0;
}

int set_absolute(tim_t dev, int channel, unsigned long long value)
{
	// currently only one channel supported
	if (dev >= MAX_TIMERS || channel > 0) {
		return -1;
	}
	ROM_TimerMatchSet((uint32_t)timer(dev), TIMER_A, value);
	timer(dev)->intr_mask |= TIMER_TIMA_MATCH;
	return 0;
}

int timer_set(tim_t dev, int channel, unsigned int timeout)
{
	return set_absolute(dev, channel, timer(dev)->timer_a + timeout);
}

int timer_set_absolute(tim_t dev, int channel, unsigned int value)
{
	return set_absolute(dev, channel, value);
}

int timer_clear(tim_t dev, int channel)
{
	// currently only one channel supported
	if (dev >= MAX_TIMERS || channel > 0) {
		return -1;
	}
	ROM_TimerIntClear((uint32_t)timer(dev), TIMER_TIMA_MATCH);
	// disable the match timer
	timer(dev)->intr_mask &= ~(TIMER_TIMA_MATCH);
	return 0;
}

unsigned int timer_read(tim_t dev)
{
	if (dev >= MAX_TIMERS) {
		return 0;
	}
	return timer(dev)->val_a;
}

void timer_start(tim_t dev)
{
	if (dev >= MAX_TIMERS) {
		return;
	}
	ROM_TimerEnable((uint32_t)timer(dev), TIMER_A);
}

void timer_stop(tim_t dev)
{
	if (dev >= MAX_TIMERS) {
		return;
	}
	ROM_TimerDisable((uint32_t)timer(dev), TIMER_A);
}

void timer_irq_enable(tim_t dev)
{
	if (dev >= MAX_TIMERS) {
		return;
	}
	ROM_TimerIntEnable((uint32_t)timer(dev), TIMER_TIMA_MATCH);
}

void timer_irq_disable(tim_t dev)
{
	if (dev >= MAX_TIMERS) {
		return;
	}
	ROM_TimerIntDisable((uint32_t)timer(dev), TIMER_TIMA_MATCH);
}

void timer_reset(tim_t dev)
{
	if (dev >= MAX_TIMERS) {
		return;
	}

	// TODO: for now only timer a is supported
	timer(dev)->val_a = 0;
}