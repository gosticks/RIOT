/*
 * Copyright (C) 2019 Ludwig Maximilian Universität
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup        cpu_cc3200_timer CC3200 General Purpose Timer
 * @ingroup         cpu_cc3200_regs
 * @{
 *
 * @file
 * @brief           Driver for the cc3200 timer controller
 *
 * Header file with register and macro declarations for the cc3200 Timer module
 *
 * @author          Wladislaw Meixner <wladislaw.meixner@campus.lmu.de>
 *
 * @{
 */

#ifndef CC3200_TIMER_H
#define CC3200_TIMER_H

#include "cc3200.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Timer modes
 */
enum {
    GPTIMER_ONE_SHOT_MODE = 1,              /**< GPTIMER one-shot mode */
    GPTIMER_PERIODIC_MODE = 2,              /**< GPTIMER periodic mode */
    GPTIMER_CAPTURE_MODE  = 3,              /**< GPTIMER capture mode */
};

/**
 * @brief Timer width configuration
 */
enum {
    GPTMCFG_32_BIT_TIMER           = 0,     /**< 32-bit timer configuration */
    GPTMCFG_32_BIT_REAL_TIME_CLOCK = 1,     /**< 32-bit real-time clock */
    GPTMCFG_16_BIT_TIMER           = 4,     /**< 16-bit timer configuration */
};

/**
 * @brief Timer interrupts
 * @{
 */
#define TIMER_TIMB_DMA 0x00002000     /**< TimerB DMA Done interrupt */
#define TIMER_TIMB_MATCH 0x00000800   /**< TimerB match interrupt */
#define TIMER_CAPB_EVENT 0x00000400   /**< CaptureB event interrupt */
#define TIMER_CAPB_MATCH 0x00000200   /**< CaptureB match interrupt */
#define TIMER_TIMB_TIMEOUT 0x00000100 /**< TimerB time out interrupt */
#define TIMER_TIMA_DMA 0x00000020     /**< TimerA DMA Done interrupt */
#define TIMER_TIMA_MATCH 0x00000010   /**< TimerA match interrupt */
#define TIMER_CAPA_EVENT 0x00000004   /**< CaptureA event interrupt */
#define TIMER_CAPA_MATCH 0x00000002   /**< CaptureA match interrupt */
#define TIMER_TIMA_TIMEOUT 0x00000001 /**< TimerA time out interrupt */
/** @} */

/**
 * @brief Timer/Subtimer selection
 * @{
 */
#define TIMER_A 0x000000ff    /**< Timer A */
#define TIMER_B 0x0000ff00    /**< Timer B */
#define TIMER_BOTH 0x0000ffff /**< Timer Both */
/** @} */

/**
 * @brief Timer configurations
 * @{
 */
#define TIMER_CFG_A_ONE_SHOT     0x00000021 /**< Timer A one-shot timer */
#define TIMER_CFG_A_ONE_SHOT_UP  0x00000031 /**< Timer A one-shot up-count timer */
#define TIMER_CFG_A_PERIODIC     0x00000022 /**< Timer A periodic timer */
#define TIMER_CFG_A_PERIODIC_UP  0x00000032 /**< Timer A periodic up-count timer */
#define TIMER_CFG_A_CAP_COUNT    0x00000003 /**< Timer A event counter */
#define TIMER_CFG_A_CAP_COUNT_UP 0x00000013 /**< Timer A event up-counter */
#define TIMER_CFG_A_CAP_TIME     0x00000007 /**< Timer A event timer */
#define TIMER_CFG_A_CAP_TIME_UP  0x00000017 /**< Timer A event up-count timer */
#define TIMER_CFG_A_PWM          0x0000000A /**< Timer A PWM output */
#define TIMER_CFG_B_ONE_SHOT     0x00002100 /**< Timer B one-shot timer */
#define TIMER_CFG_B_ONE_SHOT_UP  0x00003100 /**< Timer B one-shot up-count timer */
#define TIMER_CFG_B_PERIODIC     0x00002200 /**< Timer B periodic timer */
#define TIMER_CFG_B_PERIODIC_UP  0x00003200 /**< Timer B periodic up-count timer */
#define TIMER_CFG_B_CAP_COUNT    0x00000300 /**< Timer B event counter */
#define TIMER_CFG_B_CAP_COUNT_UP 0x00001300 /**< Timer B event up-counter */
#define TIMER_CFG_B_CAP_TIME     0x00000700 /**< Timer B event timer */
#define TIMER_CFG_B_CAP_TIME_UP  0x00001700 /**< Timer B event up-count timer */
#define TIMER_CFG_B_PWM          0x00000A00 /**< Timer B PWM output */
/** @} */

/**
 * @brief Timer component register
 * @{
 */
typedef struct {
    cc3200_reg_t conf;                /**< configuration */
    cc3200_reg_t timer_a_mode;        /**< timer A Mode */
    cc3200_reg_t timer_b_mode;        /**< timer B Mode */
    cc3200_reg_t ctrl;                /**< timer control register */
    cc3200_reg_t sync;                /**< sync */
    cc3200_reg_t RESERVED;            /**< RESERVED */
    cc3200_reg_t intr_mask;           /**< interrupt mask */
    cc3200_reg_t intr_raw_stat;       /**< raw interrupt status */
    cc3200_reg_t masked_intr;         /**< masked interrupt */
    cc3200_reg_t intr_clear;          /**< interrupt clear */
    cc3200_reg_t interval_load_a;     /**< interval load a */
    cc3200_reg_t interval_load_b;     /**< interval load b */
    cc3200_reg_t match_a;             /**< timer match a */
    cc3200_reg_t match_b;             /**< timer match b */
    cc3200_reg_t prescale_a;          /**< timer prescale a */
    cc3200_reg_t prescale_b;          /**< timer prescale b */
    cc3200_reg_t prescale_match_a;    /**< timer prescale match a */
    cc3200_reg_t prescale_match_b;    /**< timer prescale match b */
    cc3200_reg_t timer_a;             /**< timer a */
    cc3200_reg_t timer_b;             /**< timer b */
    cc3200_reg_t val_a;               /**< timer value a */
    cc3200_reg_t val_b;               /**< timer value b */
    cc3200_reg_t rtc_predivide;       /**< RTC Predivide */
    cc3200_reg_t prescale_snaphot_a;  /**< timer prescale snapshot a */
    cc3200_reg_t prescale_snapshot_b; /**< timer prescale snapshot b */
    cc3200_reg_t val_snapshot_a;      /**< timer value snapshot a */
    cc3200_reg_t val_snapshot_b;      /**< timer value snapshot b */
    cc3200_reg_t dma_event;           /**< DMA event */
    cc3200_reg_t RESERVED1[3924];     /**< Reserved */
    cc3200_reg_t pp;                  /**< GPTIMER Peripheral Properties */
} cc3200_timer_t;
/** @} */

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif /* CC3200_TIMER_H */

/** @} */