/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_oneshot.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.  See the License for the specific language governing
 * permissions and limitations under the License.
 *
 ****************************************************************************/

/* ADR-027: One-shot timer built on TIM5.
 *
 * The NuttX oneshot framework expects current() to return a monotonic,
 * free-running time reference (the test measures the delta between two
 * current() reads across a wait), while cancel() returns the time still
 * remaining.  A one-pulse-mode timer cannot satisfy both, so this uses the
 * standard free-running-counter-plus-output-compare model instead:
 *
 *   - TIM5 (32-bit) counts continuously at a 1 MHz tick (ARR = 0xffffffff),
 *     so current() is simply CNT converted to a timespec (1 tick = 1 us).
 *   - start(delay) programs CCR1 = CNT + delay_us and enables the CC1
 *     compare interrupt; on match the ISR fires the upper-half callback.
 *   - cancel() reports CCR1 - CNT as the remaining time and masks CC1.
 *
 * CCMR1 is left at its reset value so channel 1 is an output compare in
 * "frozen" mode: the compare match still sets CC1IF but drives no pin.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/clock.h>
#include <nuttx/timers/oneshot.h>
#include <nuttx/spinlock.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "chip.h"
#include "stm32n6_oneshot.h"
#include "hardware/stm32_tim.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Fixed 1 MHz tick: 1 microsecond per counter increment */

#define STM32N6_ONESHOT_TICK_FREQ  1000000

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_oneshot_lowerhalf_s
{
  struct oneshot_lowerhalf_s lh;      /* Lower-half instance (must be 1st) */
  uint32_t                   base;    /* Timer register base address */
  uint32_t                   timclk;  /* Timer input clock (Hz) */
  int                        irq;     /* Timer global IRQ number */
  bool                       running; /* True while a compare is armed */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32n6_oneshot_maxdelay(struct oneshot_lowerhalf_s *lower,
                                     struct timespec *ts);
static int stm32n6_oneshot_start(struct oneshot_lowerhalf_s *lower,
                                 const struct timespec *ts);
static int stm32n6_oneshot_cancel(struct oneshot_lowerhalf_s *lower,
                                  struct timespec *ts);
static int stm32n6_oneshot_current(struct oneshot_lowerhalf_s *lower,
                                   struct timespec *ts);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct oneshot_operations_s g_stm32n6_oneshot_ops =
{
  .max_delay = stm32n6_oneshot_maxdelay,
  .start     = stm32n6_oneshot_start,
  .cancel    = stm32n6_oneshot_cancel,
  .current   = stm32n6_oneshot_current,
};

#ifdef CONFIG_STM32_TIM5
static struct stm32n6_oneshot_lowerhalf_s g_tim5_oneshot =
{
  .lh     =
    {
      .ops = &g_stm32n6_oneshot_ops,
    },
  .base   = STM32_TIM5_BASE,
  .timclk = STM32_APB1_TIM_FREQUENCY,
  .irq    = STM32_IRQ_TIM5,
};
#endif

static spinlock_t g_oneshot_lock = SP_UNLOCKED;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_oneshot_us2ts / stm32n6_oneshot_ts2us
 *
 * Description:
 *   Convert between a microsecond count (1 tick == 1 us) and a timespec.
 *
 ****************************************************************************/

static void stm32n6_oneshot_us2ts(uint32_t usec, struct timespec *ts)
{
  ts->tv_sec  = usec / USEC_PER_SEC;
  ts->tv_nsec = (usec % USEC_PER_SEC) * NSEC_PER_USEC;
}

static uint64_t stm32n6_oneshot_ts2us(const struct timespec *ts)
{
  return (uint64_t)ts->tv_sec * USEC_PER_SEC +
         (uint64_t)ts->tv_nsec / NSEC_PER_USEC;
}

/****************************************************************************
 * Name: stm32n6_oneshot_interrupt
 *
 * Description:
 *   Compare-match interrupt handler.  Masks the compare, acknowledges the
 *   flag, and invokes the upper-half callback.
 *
 ****************************************************************************/

static int stm32n6_oneshot_interrupt(int irq, void *context, void *arg)
{
  struct stm32n6_oneshot_lowerhalf_s *priv =
    (struct stm32n6_oneshot_lowerhalf_s *)arg;
  uint32_t sr;
  irqstate_t flags;

  DEBUGASSERT(priv != NULL);

  sr = getreg32(priv->base + STM32_TIM_SR_OFFSET);
  if ((sr & TIM_SR_CC1IF) == 0)
    {
      return OK;
    }

  flags = spin_lock_irqsave(&g_oneshot_lock);

  /* Mask the compare interrupt and acknowledge the flag (rc_w0) */

  modifyreg32(priv->base + STM32_TIM_DIER_OFFSET, TIM_DIER_CC1IE, 0);
  putreg32(~TIM_SR_CC1IF, priv->base + STM32_TIM_SR_OFFSET);
  priv->running = false;

  spin_unlock_irqrestore(&g_oneshot_lock, flags);

  if (priv->lh.callback != NULL)
    {
      priv->lh.callback(&priv->lh, priv->lh.arg);
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_oneshot_maxdelay
 ****************************************************************************/

static int stm32n6_oneshot_maxdelay(struct oneshot_lowerhalf_s *lower,
                                    struct timespec *ts)
{
  DEBUGASSERT(ts != NULL);

  /* A 32-bit counter at a 1 us tick wraps after 0xffffffff microseconds */

  stm32n6_oneshot_us2ts(UINT32_MAX, ts);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_oneshot_start
 ****************************************************************************/

static int stm32n6_oneshot_start(struct oneshot_lowerhalf_s *lower,
                                 const struct timespec *ts)
{
  struct stm32n6_oneshot_lowerhalf_s *priv =
    (struct stm32n6_oneshot_lowerhalf_s *)lower;
  irqstate_t flags;
  uint64_t delay;
  uint32_t cnt;

  DEBUGASSERT(priv != NULL && ts != NULL);

  delay = stm32n6_oneshot_ts2us(ts);
  if (delay == 0 || delay > UINT32_MAX)
    {
      return -EINVAL;
    }

  flags = spin_lock_irqsave(&g_oneshot_lock);

  /* Schedule the compare relative to the free-running counter.  The 32-bit
   * add naturally wraps, matching the counter's own modulo arithmetic.
   */

  cnt = getreg32(priv->base + STM32_TIM_CNT_OFFSET);
  putreg32(cnt + (uint32_t)delay, priv->base + STM32_TIM_CCR1_OFFSET);

  /* Clear any stale compare flag, then unmask the compare interrupt */

  putreg32(~TIM_SR_CC1IF, priv->base + STM32_TIM_SR_OFFSET);
  modifyreg32(priv->base + STM32_TIM_DIER_OFFSET, 0, TIM_DIER_CC1IE);
  priv->running = true;

  spin_unlock_irqrestore(&g_oneshot_lock, flags);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_oneshot_cancel
 ****************************************************************************/

static int stm32n6_oneshot_cancel(struct oneshot_lowerhalf_s *lower,
                                  struct timespec *ts)
{
  struct stm32n6_oneshot_lowerhalf_s *priv =
    (struct stm32n6_oneshot_lowerhalf_s *)lower;
  irqstate_t flags;
  uint32_t cnt;
  uint32_t ccr;

  DEBUGASSERT(priv != NULL);

  flags = spin_lock_irqsave(&g_oneshot_lock);

  /* Mask the compare interrupt and clear any pending flag */

  modifyreg32(priv->base + STM32_TIM_DIER_OFFSET, TIM_DIER_CC1IE, 0);
  putreg32(~TIM_SR_CC1IF, priv->base + STM32_TIM_SR_OFFSET);

  cnt = getreg32(priv->base + STM32_TIM_CNT_OFFSET);
  ccr = getreg32(priv->base + STM32_TIM_CCR1_OFFSET);
  priv->running = false;

  spin_unlock_irqrestore(&g_oneshot_lock, flags);

  /* Report the time still remaining, if any.  The subtraction wraps in the
   * same modulo-2^32 space as the counter.
   */

  if (ts != NULL)
    {
      stm32n6_oneshot_us2ts(ccr - cnt, ts);
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_oneshot_current
 ****************************************************************************/

static int stm32n6_oneshot_current(struct oneshot_lowerhalf_s *lower,
                                   struct timespec *ts)
{
  struct stm32n6_oneshot_lowerhalf_s *priv =
    (struct stm32n6_oneshot_lowerhalf_s *)lower;

  DEBUGASSERT(priv != NULL && ts != NULL);

  /* The free-running counter is the monotonic time base (1 tick == 1 us) */

  stm32n6_oneshot_us2ts(getreg32(priv->base + STM32_TIM_CNT_OFFSET), ts);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_oneshot_enable
 *
 * Description:
 *   Enable the TIM5 clock (run and sleep modes), program the 1 MHz tick,
 *   and start the free-running counter.
 *
 ****************************************************************************/

static void stm32n6_oneshot_enable(struct stm32n6_oneshot_lowerhalf_s *priv)
{
  irqstate_t flags;
  uint32_t regval;
  uint32_t psc;

  flags = spin_lock_irqsave(&g_oneshot_lock);

#ifdef CONFIG_STM32_TIM5
  /* Enable the run-mode clock and, critically, the sleep-mode clock so the
   * counter keeps running while a task blocks in CPU Sleep (WFI).
   */

  regval  = getreg32(STM32_RCC_APB1ENR1);
  regval |= RCC_APB1ENR1_TIM5EN;
  putreg32(regval, STM32_RCC_APB1ENR1);

  regval  = getreg32(STM32_RCC_APB1LPENR1);
  regval |= RCC_APB1LPENR1_TIM5LPEN;
  putreg32(regval, STM32_RCC_APB1LPENR1);
#endif

  /* Stop and reset the timer while (re)configuring */

  putreg32(0, priv->base + STM32_TIM_CR1_OFFSET);
  putreg32(0, priv->base + STM32_TIM_DIER_OFFSET);

  /* 1 MHz tick and full-range auto-reload for a free-running counter */

  psc = (priv->timclk / STM32N6_ONESHOT_TICK_FREQ) - 1;
  putreg32(psc, priv->base + STM32_TIM_PSC_OFFSET);
  putreg32(UINT32_MAX, priv->base + STM32_TIM_ARR_OFFSET);

  /* CCMR1/CCER left at reset: channel 1 is an output compare in frozen
   * mode (sets CC1IF on match, drives no pin).
   */

  putreg32(0, priv->base + STM32_TIM_CCMR1_OFFSET);
  putreg32(0, priv->base + STM32_TIM_CCER_OFFSET);

  /* Load PSC/ARR and clear the resulting status, then start counting */

  putreg32(TIM_EGR_UG, priv->base + STM32_TIM_EGR_OFFSET);
  putreg32(0, priv->base + STM32_TIM_SR_OFFSET);
  putreg32(TIM_CR1_CEN, priv->base + STM32_TIM_CR1_OFFSET);

  spin_unlock_irqrestore(&g_oneshot_lock, flags);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_oneshot_initialize
 *
 * Description:
 *   Initialize a one-shot timer and return its lower-half instance for
 *   registration with the NuttX oneshot framework.
 *
 * Input Parameters:
 *   timer - Timer peripheral number (currently only 5 is supported).
 *
 * Returned Value:
 *   A pointer to the lower-half instance on success; NULL on failure.
 *
 ****************************************************************************/

struct oneshot_lowerhalf_s *stm32n6_oneshot_initialize(int timer)
{
  struct stm32n6_oneshot_lowerhalf_s *priv;

  switch (timer)
    {
#ifdef CONFIG_STM32_TIM5
      case 5:
        priv = &g_tim5_oneshot;
        break;
#endif

      default:
        return NULL;
    }

  stm32n6_oneshot_enable(priv);

  irq_attach(priv->irq, stm32n6_oneshot_interrupt, priv);
  up_enable_irq(priv->irq);

  return &priv->lh;
}
