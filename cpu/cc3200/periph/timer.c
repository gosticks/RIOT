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

#include "vendor/hw_adc.h"
#include "vendor/hw_ints.h"
#include "vendor/hw_memmap.h"
#include "vendor/hw_timer.h"
#include "vendor/hw_types.h"
#include "vendor/rom.h"

#define ENABLE_DEBUG (0)
#include "debug.h"

#define LOAD_VALUE (0xffff)

#define TIMER_A_IRQ_MASK (0x000000ff)
#define TIMER_B_IRQ_MASK (0x0000ff00)

/* GPTIMER_CTL Bits */
#define TBEN TIMER_CTL_TBEN
#define TAEN TIMER_CTL_TAEN

/* GPTIMER_TnMR Bits */
#define TNMIE TIMER_TAMR_TAMIE
#define TNCDIR TIMER_TAMR_TACDIR

typedef struct {
    uint16_t mask;
    uint16_t flag;
} _isr_cfg_t;

static const _isr_cfg_t chn_isr_cfg[] = {
    { .mask = TIMER_A_IRQ_MASK, .flag = TIMER_IMR_TAMIM },
    { .mask = TIMER_B_IRQ_MASK, .flag = TIMER_IMR_TBMIM }
};

/**
 * @brief Timer state memory
 */
static timer_isr_ctx_t isr_ctx[TIMER_NUMOF];

/* enable timer interrupts */
static inline void _irq_enable(tim_t tim)
{
    DEBUG("%s(%u)\n", __FUNCTION__, tim);

    if (tim < TIMER_NUMOF) {
        IRQn_Type irqn = GPTIMER_0A_IRQn + (2 * tim);

        NVIC_SetPriority(irqn, TIMER_IRQ_PRIO);
        NVIC_EnableIRQ(irqn);

        if (timer_config[tim].chn == 2) {
            irqn++;
            NVIC_SetPriority(irqn, TIMER_IRQ_PRIO);
            NVIC_EnableIRQ(irqn);
        }
    }
}

/**
 * @brief get timer control register by ID. Each register is 0x1000 apart
 * starting at TIMERA0_BASE
 *
 *
 */
static inline cc3200_timer_t *timer(tim_t num)
{
    return (cc3200_timer_t *)(TIMERA0_BASE + (num << 12));
}

/**
 * @brief timer interrupt handler
 *
 * @param[in] dev GPT instance number
 */
static void timer_irq_handler(tim_t tim, int channel)
{
    // DEBUG("%s(%u,%d, %u)\n", __FUNCTION__, tim, channel, timer_read(tim));
    assert(tim < TIMER_NUMOF);
    assert(channel < (int)timer_config[tim].chn);

    uint32_t mis;
    /* Latch the active interrupt flags */
    mis = timer(tim)->masked_intr & chn_isr_cfg[channel].mask;
    /* Clear the latched interrupt flags */
    timer(tim)->intr_clear = mis;

    // DEBUG("EXECUTE CALLBACK\n");
    if (mis & chn_isr_cfg[channel].flag) {
        // DEBUG("[OK]EXECUTE CALLBACK\n");
        /* Disable further match interrupts for this timer/channel */
        timer(tim)->masked_intr &= ~chn_isr_cfg[channel].flag;
        /* Invoke the callback function */
        isr_ctx[tim].cb(isr_ctx[tim].arg, channel);
        // DEBUG("[DONE]EXECUTE CALLBACK\n");
    }

    cortexm_isr_end();
}

void isr_timer0_ch_a(void)
{
    timer_irq_handler(0, 0);
};
void isr_timer0_ch_b(void)
{
    timer_irq_handler(0, 1);
};
void isr_timer1_ch_a(void)
{
    timer_irq_handler(1, 0);
};
void isr_timer1_ch_b(void)
{
    timer_irq_handler(1, 1);
};
void isr_timer2_ch_a(void)
{
    timer_irq_handler(2, 0);
};
void isr_timer2_ch_b(void)
{
    timer_irq_handler(2, 1);
};
void isr_timer3_ch_a(void)
{
    timer_irq_handler(3, 0);
};
void isr_timer3_ch_b(void)
{
    timer_irq_handler(3, 1);
};

/**
 * @brief returns the timer peripheral register used to enable or disable
 * hardware peripheral.
 *
 */
static inline cc3200_arcm_reg_t *timer_periph_reg(tim_t num)
{
    return (cc3200_arcm_reg_t *)((&ARCM->GPT_A0) +
                                 sizeof(cc3200_arcm_reg_t) * num);
}

int timer_init(tim_t tim, unsigned long freq, timer_cb_t cb, void *arg)
{
    DEBUG("%s(%u, %lu)\n", __FUNCTION__, tim, freq);
    /* check if timer id is valid */
    if (tim >= TIMER_NUMOF) {
        return -1;
    }

    /* get periph register by adding dev * sizeof(periph_reg) to the */
    /* first timer periph register */
    cc3200_timer_t *t = timer(tim);

    /* save callback function */
    isr_ctx[tim].cb  = cb;
    isr_ctx[tim].arg = arg;

    /* enable & reset periph clock */
    timer_periph_reg(tim)->clk_gating |= PRCM_RUN_MODE_CLK;
    reset_periph_clk(timer_periph_reg(tim));

    /* Enable CCP to IO path */
    HWREG(APPS_CONFIG_BASE + ADC_O_adc_ch7_fifo_lvl) = 0xFF;

    DEBUG("DEVICE REGISTER 0x%lx \n", (uint32_t)t);
    /* disable the timer */
    DEBUG("DEVICE REGISTER 0x%lx 0x%lx \n", (uint32_t)t, (uint32_t)t->ctrl);
    t->ctrl &= 0;
    DEBUG("DEVICE REGISTER 0x%lx 0x%lx \n", (uint32_t)t, (uint32_t)t->ctrl);

    uint32_t prescaler = 0;
    uint32_t chan_mode = TNMIE | TIMER_TAMR_TAMR_PERIOD;
    if (timer_config[tim].cfg == TIMER_CFG_32_BIT_TIMER) {
        /* Count up in periodic mode */
        chan_mode |= TNCDIR;

        if (timer_config[tim].chn > 1) {
            DEBUG("Invalid timer_config. Multiple channels are available only in 16-bit mode.");
            return -1;
        }

        if (freq != CLOCK_CORECLOCK) {
            DEBUG("In 32-bit mode, the GPTimer frequency must equal the system clock frequency (%u).\n",
                  (unsigned)CLOCK_CORECLOCK);
            return -1;
        }
    } else if (timer_config[tim].cfg == TIMER_CFG_16_BIT) {
        prescaler = CLOCK_CORECLOCK;
        prescaler += freq / 2;
        prescaler /= freq;
        if (prescaler > 0)
            prescaler--;
        if (prescaler > 255)
            prescaler = 255;
        DEBUG("timer_init: 16 bit timer prescaler=%lu \n", prescaler);

        t->prescale_a      = prescaler;
        t->interval_load_a = LOAD_VALUE;
    } else {
        DEBUG("timer_init: invalid timer config must be 16 or 32Bit mode!\n");
        return -1;
    }

    t->conf = timer_config[tim].cfg; // | TIMER_CFG_A_PERIODIC_UP >> 24;
    t->ctrl &= TAEN;
    t->timer_a_mode = chan_mode & 255;

    if (timer_config[tim].chn > 1) {
        t->timer_b_mode    = chan_mode & 255;
        t->prescale_b      = prescaler;
        t->interval_load_b = LOAD_VALUE;
        /* Enable the timer: */
        t->ctrl &= TBEN;
    }

    ROM_TimerConfigure((uint32_t)timer(tim), 0x04000000 | TIMER_CFG_A_PERIODIC);
    ROM_TimerPrescaleSet((uint32_t)timer(tim), TIMER_A, 79);
    ROM_TimerIntRegister((uint32_t)timer(tim), TIMER_A, isr_timer0_ch_a);
    // ROM_TimerControlStall((uint32_t)timer(tim), TIMER_A, true);

    ROM_IntPriorityGroupingSet(3);
    ROM_IntPrioritySet(INT_TIMERA0A, 0xFF);

    // ROM_TimerLoadSet((uint32_t)timer(tim), TIMER_A, 5000);
    ROM_TimerIntEnable((uint32_t)timer(tim), TIMER_CAPA_MATCH);

    ROM_TimerEnable((uint32_t)timer(tim), TIMER_A);
    // _irq_enable(tim);
    return 0;
}

int timer_set_absolute(tim_t tim, int channel, unsigned int value)
{
    DEBUG("timer_set_absolute(%u, %u, %u)\n", tim, channel, value);

    // if (tim >= TIMER_NUMOF || channel >= (int)timer_config[tim].chn) {
    //     return -1;
    // }
    /* clear any pending match interrupts */
    timer(tim)->intr_clear = chn_isr_cfg[channel].flag;
    if (channel == 0) {
        ROM_TimerMatchSet((uint32_t)timer(tim), TIMER_A, (uint32_t)timer(tim));
        // printf("timer_set_absolute(%u, %u)\n", (uint16_t)timer(tim)->val_a,
        //        value);

    } else {
        timer(tim)->match_b =
                (timer_config[tim].cfg == TIMER_CFG_32_BIT_TIMER) ?
                        value :
                        (LOAD_VALUE - value);
    }
    timer(tim)->intr_mask |= chn_isr_cfg[channel].flag;

    return 1;
}

int timer_clear(tim_t tim, int channel)
{
    DEBUG("%s(%u, %u)\n", __FUNCTION__, tim, channel);

    if ((tim >= TIMER_NUMOF) || (channel >= (int)timer_config[tim].chn)) {
        return -1;
    }
    /* disable the match timer */
    timer(tim)->intr_mask &= ~(TIMER_TIMA_MATCH);
    return 0;
}

unsigned int timer_read(tim_t tim)
{
    if (tim >= TIMER_NUMOF) {
        return 0;
    }
    // while (true) {
    //     DEBUG("a= %d  | b= %ld \n", (uint16_t)timer(tim)->val_a,
    //           (uint32_t)timer(tim)->val_b);
    // }

    if (timer_config[tim].cfg == TIMER_CFG_32_BIT_TIMER) {
        return timer(tim)->val_a;
    } else {
        return (uint16_t)(timer(tim)->val_a);
    }
}

void timer_start(tim_t tim)
{
    DEBUG("%s(%u)\n", __FUNCTION__, tim);

    if (tim < TIMER_NUMOF) {
        if (timer_config[tim].chn == 1) {
            timer(tim)->ctrl = TAEN;
        } else if (timer_config[tim].chn == 2) {
            timer(tim)->ctrl = TBEN | TAEN;
        }
    }
}

void timer_stop(tim_t tim)
{
    DEBUG("%s(%u)\n", __FUNCTION__, tim);

    if (tim < TIMER_NUMOF) {
        timer(tim)->ctrl = 0;
    }
}
