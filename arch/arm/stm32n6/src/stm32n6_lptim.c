/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_lptim.c
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

/* ADR-028: Low-power timer (LPTIM1-5) lower-half driver.
 *
 * This delivers the timing function of ADR-028 via two NuttX frameworks:
 * LPTIM1-5 as plain periodic timers on the timer character framework
 * (/dev/timerN), and -- when CONFIG_STM32_LPTIM2_PWM is set -- LPTIM2 as a
 * low-power PWM output on the PWM framework (/dev/pwm0) driving LPTIM2_CH1.
 * Each LPTIM is a 16-bit counter clocked here from the LSI (~32 kHz) so it
 * keeps running from a low-power oscillator independent of the APB clock.
 *
 * PWM mode shares the LPTIM CR/CFGR with periodic mode, so a given instance
 * is exposed as either /dev/timerN or /dev/pwm0, never both; the Kconfig
 * choice enforces the mutual exclusion (LPTIM2_PWM depends on !LPTIM2).
 *
 * The remaining low-power differentiator -- keeping the counter running
 * through CPU Stop mode to wake the core -- is left as a separate ADR-028
 * sub-item.
 *
 * LPTIM differs from the general-purpose timers in its programming model:
 * CFGR and the interrupt-enable register must be written while the timer
 * is disabled; the auto-reload register must be written only after the
 * timer is enabled and each write must wait for the ISR.ARROK handshake;
 * and the periodic event is the auto-reload match (ARRM), not the TIM
 * update flag.  Because the counter clock (LSI) is asynchronous to the
 * APB read bus, the counter is read coherently (two matching reads).
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/timers/timer.h>
#include <nuttx/spinlock.h>

#ifdef CONFIG_STM32_LPTIM2_PWM
#  include <nuttx/timers/pwm.h>
#  include "stm32n6_gpio.h"
#endif

#include "arm_internal.h"
#include "chip.h"
#include "stm32n6_lptim.h"
#include "hardware/stm32_lptim.h"
#include "hardware/stm32_rcc.h"

#ifdef CONFIG_STM32_LPTIM_STOPWAKE
#  include <time.h>
#  include "stm32n6_pwr.h"
#  include "hardware/stm32_exti.h"
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* LSI nominal frequency (Hz).  The LPTIM tick equals one LSI period with
 * the prescaler left at divide-by-1, i.e. ~31.25 us.
 */

#define STM32N6_LPTIM_LSI_FREQ  32000

/* 16-bit counter: the largest reload is 0x10000 ticks, so the maximum
 * timeout is that many LSI periods expressed in microseconds.
 */

#define STM32N6_LPTIM_MAXTICKS  0x10000ull
#define STM32N6_LPTIM_MAXTIMEOUT \
  ((STM32N6_LPTIM_MAXTICKS * 1000000ull) / STM32N6_LPTIM_LSI_FREQ)

/* Bounded handshake budgets (microseconds).  LSI enable and the ARROK
 * handshake each take only a few slow-clock cycles; never spin forever.
 */

#define STM32N6_LPTIM_TIMEOUT_US  10000

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_lptim_lowerhalf_s
{
  const struct timer_ops_s *ops;      /* Lower-half ops (must be 1st) */
  uint32_t                  base;     /* LPTIM register base address */
  int                       irq;      /* LPTIM global IRQ number */

  /* Per-instance clocking.  LPTIM1 sits on APB1; LPTIM2-5 sit on APB4, so
   * the enable / sleep-keep-alive registers and the CCIPR12 kernel-clock
   * select field differ per instance.  These make enableclk() data-driven.
   */

  uint32_t                  clken;    /* RCC bus clock-enable register */
  uint32_t                  clkbit;   /* Enable bit within clken */
  uint32_t                  lpen;     /* RCC sleep clock-enable register */
  uint32_t                  lpbit;    /* Keep-alive bit within lpen */
  uint32_t                  selmask;  /* CCIPR12 kernel-clock select mask */
  uint32_t                  sellsi;   /* CCIPR12 LSI select value */

  tccb_t                    callback; /* Upper-half timeout callback */
  void                     *arg;      /* Argument for the callback */
  uint32_t                  timeout;  /* Current timeout (microseconds) */
  bool                      started;  /* True when the timer is running */
};

#ifdef CONFIG_STM32_LPTIM2_PWM

/* LPTIM2 PWM-output lower-half.  PWM mode is mutually exclusive with the
 * plain periodic-timer mode (they share CR/CFGR), so a given LPTIM instance
 * is exposed either as /dev/timerN or as /dev/pwm0, never both.
 */

struct stm32n6_lptim_pwm_s
{
  const struct pwm_ops_s *ops;      /* Lower-half ops (must be 1st) */
  uint32_t                base;     /* LPTIM register base address */
  uint32_t                pin;      /* LPTIM_CH1 output pin config */

  uint32_t                clken;    /* RCC bus clock-enable register */
  uint32_t                clkbit;   /* Enable bit within clken */
  uint32_t                lpen;     /* RCC sleep clock-enable register */
  uint32_t                lpbit;    /* Keep-alive bit within lpen */
  uint32_t                selmask;  /* CCIPR12 kernel-clock select mask */
  uint32_t                sellsi;   /* CCIPR12 LSI select value */
};
#endif

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int  stm32n6_lptim_start(struct timer_lowerhalf_s *lower);
static int  stm32n6_lptim_stop(struct timer_lowerhalf_s *lower);
static int  stm32n6_lptim_getstatus(struct timer_lowerhalf_s *lower,
                                     struct timer_status_s *status);
static int  stm32n6_lptim_settimeout(struct timer_lowerhalf_s *lower,
                                      uint32_t timeout);
static void stm32n6_lptim_setcallback(struct timer_lowerhalf_s *lower,
                                       tccb_t callback, void *arg);
static int  stm32n6_lptim_maxtimeout(struct timer_lowerhalf_s *lower,
                                      uint32_t *maxtimeout);

#ifdef CONFIG_STM32_LPTIM2_PWM
static int  stm32n6_lppwm_setup(struct pwm_lowerhalf_s *dev);
static int  stm32n6_lppwm_shutdown(struct pwm_lowerhalf_s *dev);
static int  stm32n6_lppwm_start(struct pwm_lowerhalf_s *dev,
                                const struct pwm_info_s *info);
static int  stm32n6_lppwm_stop(struct pwm_lowerhalf_s *dev);
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct timer_ops_s g_stm32n6_lptim_ops =
{
  .start       = stm32n6_lptim_start,
  .stop        = stm32n6_lptim_stop,
  .getstatus   = stm32n6_lptim_getstatus,
  .settimeout  = stm32n6_lptim_settimeout,
  .setcallback = stm32n6_lptim_setcallback,
  .maxtimeout  = stm32n6_lptim_maxtimeout,
};

#ifdef CONFIG_STM32_LPTIM1
static struct stm32n6_lptim_lowerhalf_s g_lptim1_lowerhalf =
{
  .ops     = &g_stm32n6_lptim_ops,
  .base    = STM32_LPTIM1_BASE,
  .irq     = STM32_IRQ_LPTIM1,
  .clken   = STM32_RCC_APB1ENR1,
  .clkbit  = RCC_APB1ENR1_LPTIM1EN,
  .lpen    = STM32_RCC_APB1LPENR1,
  .lpbit   = RCC_APB1LPENR1_LPTIM1LPEN,
  .selmask = RCC_CCIPR12_LPTIM1SEL_MASK,
  .sellsi  = RCC_CCIPR12_LPTIM1SEL_LSI,
};
#endif

#ifdef CONFIG_STM32_LPTIM2
static struct stm32n6_lptim_lowerhalf_s g_lptim2_lowerhalf =
{
  .ops     = &g_stm32n6_lptim_ops,
  .base    = STM32_LPTIM2_BASE,
  .irq     = STM32_IRQ_LPTIM2,
  .clken   = STM32_RCC_APB4ENR1,
  .clkbit  = RCC_APB4ENR1_LPTIM2EN,
  .lpen    = STM32_RCC_APB4LPENR1,
  .lpbit   = RCC_APB4LPENR1_LPTIM2LPEN,
  .selmask = RCC_CCIPR12_LPTIM2SEL_MASK,
  .sellsi  = RCC_CCIPR12_LPTIM2SEL_LSI,
};
#endif

#ifdef CONFIG_STM32_LPTIM3
static struct stm32n6_lptim_lowerhalf_s g_lptim3_lowerhalf =
{
  .ops     = &g_stm32n6_lptim_ops,
  .base    = STM32_LPTIM3_BASE,
  .irq     = STM32_IRQ_LPTIM3,
  .clken   = STM32_RCC_APB4ENR1,
  .clkbit  = RCC_APB4ENR1_LPTIM3EN,
  .lpen    = STM32_RCC_APB4LPENR1,
  .lpbit   = RCC_APB4LPENR1_LPTIM3LPEN,
  .selmask = RCC_CCIPR12_LPTIM3SEL_MASK,
  .sellsi  = RCC_CCIPR12_LPTIM3SEL_LSI,
};
#endif

#ifdef CONFIG_STM32_LPTIM4
static struct stm32n6_lptim_lowerhalf_s g_lptim4_lowerhalf =
{
  .ops     = &g_stm32n6_lptim_ops,
  .base    = STM32_LPTIM4_BASE,
  .irq     = STM32_IRQ_LPTIM4,
  .clken   = STM32_RCC_APB4ENR1,
  .clkbit  = RCC_APB4ENR1_LPTIM4EN,
  .lpen    = STM32_RCC_APB4LPENR1,
  .lpbit   = RCC_APB4LPENR1_LPTIM4LPEN,
  .selmask = RCC_CCIPR12_LPTIM4SEL_MASK,
  .sellsi  = RCC_CCIPR12_LPTIM4SEL_LSI,
};
#endif

#ifdef CONFIG_STM32_LPTIM5
static struct stm32n6_lptim_lowerhalf_s g_lptim5_lowerhalf =
{
  .ops     = &g_stm32n6_lptim_ops,
  .base    = STM32_LPTIM5_BASE,
  .irq     = STM32_IRQ_LPTIM5,
  .clken   = STM32_RCC_APB4ENR1,
  .clkbit  = RCC_APB4ENR1_LPTIM5EN,
  .lpen    = STM32_RCC_APB4LPENR1,
  .lpbit   = RCC_APB4LPENR1_LPTIM5LPEN,
  .selmask = RCC_CCIPR12_LPTIM5SEL_MASK,
  .sellsi  = RCC_CCIPR12_LPTIM5SEL_LSI,
};
#endif

#ifdef CONFIG_STM32_LPTIM2_PWM
static const struct pwm_ops_s g_stm32n6_lppwm_ops =
{
  .setup    = stm32n6_lppwm_setup,
  .shutdown = stm32n6_lppwm_shutdown,
  .start    = stm32n6_lppwm_start,
  .stop     = stm32n6_lppwm_stop,
};

static struct stm32n6_lptim_pwm_s g_lptim2_pwm =
{
  .ops     = &g_stm32n6_lppwm_ops,
  .base    = STM32_LPTIM2_BASE,
  .pin     = GPIO_LPTIM2_CH1,
  .clken   = STM32_RCC_APB4ENR1,
  .clkbit  = RCC_APB4ENR1_LPTIM2EN,
  .lpen    = STM32_RCC_APB4LPENR1,
  .lpbit   = RCC_APB4LPENR1_LPTIM2LPEN,
  .selmask = RCC_CCIPR12_LPTIM2SEL_MASK,
  .sellsi  = RCC_CCIPR12_LPTIM2SEL_LSI,
};
#endif

static spinlock_t g_lptim_lock = SP_UNLOCKED;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_lptim_timeout_to_ticks
 *
 * Description:
 *   Convert a timeout in microseconds to LPTIM counter ticks (LSI periods).
 *   Returns the tick count, or 0 if the request is out of the valid
 *   [1, 0x10000] range.
 *
 ****************************************************************************/

static uint32_t stm32n6_lptim_timeout_to_ticks(uint32_t timeout)
{
  uint64_t ticks;

  ticks = ((uint64_t)timeout * STM32N6_LPTIM_LSI_FREQ) / 1000000ull;
  if (ticks == 0 || ticks > STM32N6_LPTIM_MAXTICKS)
    {
      return 0;
    }

  return (uint32_t)ticks;
}

/****************************************************************************
 * Name: stm32n6_lptim_readcnt
 *
 * Description:
 *   Read the LPTIM counter coherently.  The counter is clocked by the LSI,
 *   which is asynchronous to the APB read bus, so a single read can catch
 *   the register mid-carry; the reference manual requires reading until two
 *   consecutive reads agree.
 *
 ****************************************************************************/

static uint32_t stm32n6_lptim_readcnt(struct stm32n6_lptim_lowerhalf_s *priv)
{
  uint32_t v1;
  uint32_t v2;

  v1 = getreg32(priv->base + STM32_LPTIM_CNT_OFFSET);
  do
    {
      v2 = v1;
      v1 = getreg32(priv->base + STM32_LPTIM_CNT_OFFSET);
    }
  while (v1 != v2);

  return v1 & 0xffff;
}

/****************************************************************************
 * Name: stm32n6_lptim_enableclk
 *
 * Description:
 *   Bring up the LPTIM1 clocking: enable the LSI oscillator, route it to
 *   the LPTIM1 kernel-clock mux, open the APB1 peripheral gate and keep it
 *   alive across CPU Sleep (WFI).  Without the LPEN bit the clock gates
 *   while a task blocks and the periodic interrupt never wakes the core.
 *
 ****************************************************************************/

static int stm32n6_lptim_clk_bringup(uint32_t clken, uint32_t clkbit,
                                     uint32_t lpen, uint32_t lpbit,
                                     uint32_t selmask, uint32_t sellsi)
{
  irqstate_t flags;
  uint32_t regval;
  int i;
  int ret = -ETIMEDOUT;

  flags = spin_lock_irqsave(&g_lptim_lock);

  /* Enable LSI (write to the CSR set-alias) and wait for it to stabilize */

  putreg32(RCC_CR_LSION, STM32_RCC_CSR);

  for (i = 0; i < STM32N6_LPTIM_TIMEOUT_US; i++)
    {
      if ((getreg32(STM32_RCC_SR) & RCC_SR_LSIRDY) != 0)
        {
          ret = OK;
          break;
        }

      up_udelay(1);
    }

  if (ret < 0)
    {
      spin_unlock_irqrestore(&g_lptim_lock, flags);
      return ret;
    }

  /* Select LSI as this LPTIM's kernel clock (CCIPR12 carries all five) */

  regval  = getreg32(STM32_RCC_CCIPR12);
  regval &= ~selmask;
  regval |= sellsi;
  putreg32(regval, STM32_RCC_CCIPR12);

  /* Open the peripheral bus gate and its Sleep-mode keep-alive.  LPTIM1
   * lives on APB1, LPTIM2-5 on APB4; the register/bit pair is per instance.
   */

  regval  = getreg32(clken);
  regval |= clkbit;
  putreg32(regval, clken);

  regval  = getreg32(lpen);
  regval |= lpbit;
  putreg32(regval, lpen);

  spin_unlock_irqrestore(&g_lptim_lock, flags);

  return OK;
}

static int stm32n6_lptim_enableclk(struct stm32n6_lptim_lowerhalf_s *priv)
{
  return stm32n6_lptim_clk_bringup(priv->clken, priv->clkbit,
                                   priv->lpen, priv->lpbit,
                                   priv->selmask, priv->sellsi);
}

/****************************************************************************
 * Name: stm32n6_lptim_write_arr
 *
 * Description:
 *   Program the auto-reload register while the timer is enabled and wait
 *   for the ISR.ARROK handshake, bounded by a timeout so it can never spin
 *   forever.  ARR must only be written with the timer enabled.
 *
 ****************************************************************************/

static int stm32n6_lptim_write_arr(struct stm32n6_lptim_lowerhalf_s *priv,
                                    uint32_t ticks)
{
  int i;

  /* Clear a stale ARROK, program (ticks - 1), then wait for ARROK */

  putreg32(LPTIM_ICR_ARROKCF, priv->base + STM32_LPTIM_ICR_OFFSET);
  putreg32(ticks - 1, priv->base + STM32_LPTIM_ARR_OFFSET);

  for (i = 0; i < STM32N6_LPTIM_TIMEOUT_US; i++)
    {
      if ((getreg32(priv->base + STM32_LPTIM_ISR_OFFSET) &
           LPTIM_ISR_ARROK) != 0)
        {
          putreg32(LPTIM_ICR_ARROKCF, priv->base + STM32_LPTIM_ICR_OFFSET);
          return OK;
        }

      up_udelay(1);
    }

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: stm32n6_lptim_interrupt
 *
 * Description:
 *   Auto-reload-match interrupt handler.  Acknowledges ARRM and invokes the
 *   registered upper-half callback.  A non-reloading (or NULL) callback
 *   stops the timer.
 *
 ****************************************************************************/

static int stm32n6_lptim_interrupt(int irq, void *context, void *arg)
{
  struct stm32n6_lptim_lowerhalf_s *priv =
    (struct stm32n6_lptim_lowerhalf_s *)arg;
  uint32_t isr;

  DEBUGASSERT(priv != NULL);

  isr = getreg32(priv->base + STM32_LPTIM_ISR_OFFSET);
  if ((isr & LPTIM_ISR_ARRM) == 0)
    {
      return OK;
    }

  /* Acknowledge the auto-reload match */

  putreg32(LPTIM_ICR_ARRMCF, priv->base + STM32_LPTIM_ICR_OFFSET);

  if (priv->callback != NULL)
    {
      uint32_t next = priv->timeout;

      if (priv->callback(&next, priv->arg))
        {
          /* Honour a possibly-updated interval */

          if (next != priv->timeout)
            {
              stm32n6_lptim_settimeout(
                (struct timer_lowerhalf_s *)priv, next);
            }
        }
      else
        {
          stm32n6_lptim_stop((struct timer_lowerhalf_s *)priv);
        }
    }
  else
    {
      stm32n6_lptim_stop((struct timer_lowerhalf_s *)priv);
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_lptim_start
 ****************************************************************************/

static int stm32n6_lptim_start(struct timer_lowerhalf_s *lower)
{
  struct stm32n6_lptim_lowerhalf_s *priv =
    (struct stm32n6_lptim_lowerhalf_s *)lower;
  irqstate_t flags;
  uint32_t ticks;
  int ret;

  DEBUGASSERT(priv != NULL);

  if (priv->started)
    {
      return -EBUSY;
    }

  ticks = stm32n6_lptim_timeout_to_ticks(priv->timeout);
  if (ticks == 0)
    {
      return -EINVAL;
    }

  /* Bring up LSI + kernel clock + APB gate before touching any register */

  ret = stm32n6_lptim_enableclk(priv);
  if (ret < 0)
    {
      tmrerr("ERROR: LPTIM LSI clock enable timeout\n");
      return ret;
    }

  flags = spin_lock_irqsave(&g_lptim_lock);

  /* Configuration and interrupt-enable registers may only be written while
   * the timer is disabled.  Use the internal (kernel) clock with the
   * prescaler at divide-by-1 and enable ARR preload for glitch-free
   * updates, then arm the auto-reload-match interrupt.
   */

  putreg32(0, priv->base + STM32_LPTIM_CR_OFFSET);
  putreg32(LPTIM_CFGR_PRELOAD, priv->base + STM32_LPTIM_CFGR_OFFSET);
  putreg32(LPTIM_DIER_ARRMIE, priv->base + STM32_LPTIM_DIER_OFFSET);

  /* Enable the timer; ARR can only be programmed once enabled */

  putreg32(LPTIM_CR_ENABLE, priv->base + STM32_LPTIM_CR_OFFSET);

  ret = stm32n6_lptim_write_arr(priv, ticks);
  if (ret < 0)
    {
      putreg32(0, priv->base + STM32_LPTIM_CR_OFFSET);
      spin_unlock_irqrestore(&g_lptim_lock, flags);
      tmrerr("ERROR: LPTIM ARR update (ARROK) timeout\n");
      return ret;
    }

  /* Drop any pending match flag, then start counting continuously */

  putreg32(LPTIM_ICR_ARRMCF, priv->base + STM32_LPTIM_ICR_OFFSET);
  modifyreg32(priv->base + STM32_LPTIM_CR_OFFSET, 0, LPTIM_CR_CNTSTRT);

  priv->started = true;

  spin_unlock_irqrestore(&g_lptim_lock, flags);

  up_enable_irq(priv->irq);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_lptim_stop
 ****************************************************************************/

static int stm32n6_lptim_stop(struct timer_lowerhalf_s *lower)
{
  struct stm32n6_lptim_lowerhalf_s *priv =
    (struct stm32n6_lptim_lowerhalf_s *)lower;
  irqstate_t flags;

  DEBUGASSERT(priv != NULL);

  if (!priv->started)
    {
      return -EINVAL;
    }

  flags = spin_lock_irqsave(&g_lptim_lock);

  /* Clearing ENABLE stops and resets the counter.  Disable the interrupt
   * (writable now that the timer is disabled) and drop any pending flag.
   */

  putreg32(0, priv->base + STM32_LPTIM_CR_OFFSET);
  putreg32(0, priv->base + STM32_LPTIM_DIER_OFFSET);
  putreg32(LPTIM_ICR_ARRMCF, priv->base + STM32_LPTIM_ICR_OFFSET);

  priv->started = false;

  spin_unlock_irqrestore(&g_lptim_lock, flags);

  up_disable_irq(priv->irq);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_lptim_getstatus
 ****************************************************************************/

static int stm32n6_lptim_getstatus(struct timer_lowerhalf_s *lower,
                                    struct timer_status_s *status)
{
  struct stm32n6_lptim_lowerhalf_s *priv =
    (struct stm32n6_lptim_lowerhalf_s *)lower;
  uint32_t arr;
  uint32_t cnt;
  uint32_t left;

  DEBUGASSERT(priv != NULL && status != NULL);

  status->flags = 0;
  if (priv->started)
    {
      status->flags |= TCFLAGS_ACTIVE;
    }

  if (priv->callback != NULL)
    {
      status->flags |= TCFLAGS_HANDLER;
    }

  status->timeout = priv->timeout;

  /* Convert the remaining ticks back to microseconds (1 tick = LSI period) */

  arr  = getreg32(priv->base + STM32_LPTIM_ARR_OFFSET) & 0xffff;
  cnt  = stm32n6_lptim_readcnt(priv);
  left = (arr >= cnt) ? (arr - cnt) : 0;

  status->timeleft =
    (uint32_t)(((uint64_t)left * 1000000ull) / STM32N6_LPTIM_LSI_FREQ);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_lptim_settimeout
 ****************************************************************************/

static int stm32n6_lptim_settimeout(struct timer_lowerhalf_s *lower,
                                     uint32_t timeout)
{
  struct stm32n6_lptim_lowerhalf_s *priv =
    (struct stm32n6_lptim_lowerhalf_s *)lower;
  irqstate_t flags;
  uint32_t ticks;
  int ret = OK;

  DEBUGASSERT(priv != NULL);

  ticks = stm32n6_lptim_timeout_to_ticks(timeout);
  if (ticks == 0)
    {
      return -EINVAL;
    }

  flags = spin_lock_irqsave(&g_lptim_lock);

  priv->timeout = timeout;

  /* If the timer is already running, reprogram the auto-reload live (the
   * PRELOAD bit defers it to the next period).  Otherwise start() applies
   * the reload once the timer is enabled.
   */

  if (priv->started)
    {
      ret = stm32n6_lptim_write_arr(priv, ticks);
    }

  spin_unlock_irqrestore(&g_lptim_lock, flags);

  return ret;
}

/****************************************************************************
 * Name: stm32n6_lptim_setcallback
 ****************************************************************************/

static void stm32n6_lptim_setcallback(struct timer_lowerhalf_s *lower,
                                       tccb_t callback, void *arg)
{
  struct stm32n6_lptim_lowerhalf_s *priv =
    (struct stm32n6_lptim_lowerhalf_s *)lower;
  irqstate_t flags;

  DEBUGASSERT(priv != NULL);

  flags = spin_lock_irqsave(&g_lptim_lock);
  priv->callback = callback;
  priv->arg      = arg;
  spin_unlock_irqrestore(&g_lptim_lock, flags);
}

/****************************************************************************
 * Name: stm32n6_lptim_maxtimeout
 ****************************************************************************/

static int stm32n6_lptim_maxtimeout(struct timer_lowerhalf_s *lower,
                                    uint32_t *maxtimeout)
{
  UNUSED(lower);
  DEBUGASSERT(maxtimeout != NULL);

  *maxtimeout = (uint32_t)STM32N6_LPTIM_MAXTIMEOUT;
  return OK;
}

#ifdef CONFIG_STM32_LPTIM2_PWM

/****************************************************************************
 * Name: stm32n6_lppwm_write_reg16
 *
 * Description:
 *   Write a 16-bit ARR or CCR1 value while the timer is enabled and wait
 *   for its update-OK handshake flag (ARROK for ARR, CMP1OK for CCR1),
 *   bounded by a timeout so it can never spin forever.  These flags only
 *   assert when the peripheral clock is running and the write has
 *   propagated across the LSI domain, so a PASS is real-hardware evidence
 *   the register took effect.
 *
 ****************************************************************************/

static int stm32n6_lppwm_write_reg16(struct stm32n6_lptim_pwm_s *priv,
                                     uint32_t offset, uint32_t value,
                                     uint32_t okflag, uint32_t okclr)
{
  int i;

  putreg32(okclr, priv->base + STM32_LPTIM_ICR_OFFSET);
  putreg32(value, priv->base + offset);

  for (i = 0; i < STM32N6_LPTIM_TIMEOUT_US; i++)
    {
      if ((getreg32(priv->base + STM32_LPTIM_ISR_OFFSET) & okflag) != 0)
        {
          putreg32(okclr, priv->base + STM32_LPTIM_ICR_OFFSET);
          return OK;
        }

      up_udelay(1);
    }

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: stm32n6_lppwm_setup
 *
 * Description:
 *   Bring up LPTIM2's LSI kernel clock + APB4 gate and route the CH1 output
 *   pin.  Called when /dev/pwm0 is opened; no pulses are emitted yet.
 *
 ****************************************************************************/

static int stm32n6_lppwm_setup(struct pwm_lowerhalf_s *dev)
{
  struct stm32n6_lptim_pwm_s *priv = (struct stm32n6_lptim_pwm_s *)dev;
  int ret;

  DEBUGASSERT(priv != NULL);

  ret = stm32n6_lptim_clk_bringup(priv->clken, priv->clkbit,
                                  priv->lpen, priv->lpbit,
                                  priv->selmask, priv->sellsi);
  if (ret < 0)
    {
      pwmerr("ERROR: LPTIM2 LSI clock enable timeout\n");
      return ret;
    }

  stm32n6_configgpio(priv->pin);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_lppwm_shutdown
 ****************************************************************************/

static int stm32n6_lppwm_shutdown(struct pwm_lowerhalf_s *dev)
{
  struct stm32n6_lptim_pwm_s *priv = (struct stm32n6_lptim_pwm_s *)dev;

  DEBUGASSERT(priv != NULL);

  stm32n6_lppwm_stop(dev);
  stm32n6_unconfiggpio(priv->pin);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_lppwm_start
 *
 * Description:
 *   Program the requested frequency + duty and start the continuous PWM.
 *   Register order follows ST's HAL: configure while disabled, enable, write
 *   ARR (poll ARROK) then CCR1 (poll CMP1OK), enable the CH1 output, then
 *   start continuous counting.  For LPTIM2 the output polarity lives in
 *   CCMR1.CC1P (LPTIM4/5 would use CFGR.WAVPOL instead).
 *
 ****************************************************************************/

static int stm32n6_lppwm_start(struct pwm_lowerhalf_s *dev,
                               const struct pwm_info_s *info)
{
  struct stm32n6_lptim_pwm_s *priv = (struct stm32n6_lptim_pwm_s *)dev;
  irqstate_t flags;
  uint32_t arr;
  uint32_t ccr;
  int ret;

  DEBUGASSERT(priv != NULL && info != NULL);

  /* 16-bit counter at the divide-by-1 LSI tick: ARR = LSI/freq - 1 must land
   * in [1, 0xffff], so the frequency must be within (LSI/65536, LSI/2].
   */

  if (info->frequency == 0 ||
      info->frequency > (STM32N6_LPTIM_LSI_FREQ / 2))
    {
      return -EINVAL;
    }

  arr = (STM32N6_LPTIM_LSI_FREQ / info->frequency);
  if (arr < 2 || arr > (STM32N6_LPTIM_MAXTICKS))
    {
      return -EINVAL;
    }

  arr -= 1;

  /* Duty is a b16 fraction of the full period (65536 == 100%).  CCR1 sets
   * the compare point within [0, ARR]; clamp so 100% stays in range.
   */

  ccr = (uint32_t)(((uint64_t)(arr + 1) * info->duty) >> 16);
  if (ccr > arr)
    {
      ccr = arr;
    }

  flags = spin_lock_irqsave(&g_lptim_lock);

  /* CFGR / CCMR1 polarity are writable only while disabled.  Internal
   * clock, prescaler divide-by-1, ARR preload, PWM waveform (WAVE = 0).
   * No interrupt is needed: the hardware drives the CH1 waveform on its own.
   */

  putreg32(0, priv->base + STM32_LPTIM_CR_OFFSET);
  putreg32(LPTIM_CFGR_PRELOAD, priv->base + STM32_LPTIM_CFGR_OFFSET);
  putreg32(0, priv->base + STM32_LPTIM_CCMR1_OFFSET);

  /* Enable; ARR and CCR1 can only be programmed once the timer is enabled */

  putreg32(LPTIM_CR_ENABLE, priv->base + STM32_LPTIM_CR_OFFSET);

  ret = stm32n6_lppwm_write_reg16(priv, STM32_LPTIM_ARR_OFFSET, arr,
                                  LPTIM_ISR_ARROK, LPTIM_ICR_ARROKCF);
  if (ret < 0)
    {
      goto err;
    }

  ret = stm32n6_lppwm_write_reg16(priv, STM32_LPTIM_CCR1_OFFSET, ccr,
                                  LPTIM_ISR_CMP1OK, LPTIM_ICR_CMP1OKCF);
  if (ret < 0)
    {
      goto err;
    }

  /* Enable the CH1 compare output (CC1SEL = 0 keeps it an output) and start
   * counting continuously.  Clear any stale match before arming the counter.
   */

  modifyreg32(priv->base + STM32_LPTIM_CCMR1_OFFSET, 0, LPTIM_CCMR1_CC1E);
  modifyreg32(priv->base + STM32_LPTIM_CR_OFFSET, 0, LPTIM_CR_CNTSTRT);

  spin_unlock_irqrestore(&g_lptim_lock, flags);
  return OK;

err:
  putreg32(0, priv->base + STM32_LPTIM_CR_OFFSET);
  spin_unlock_irqrestore(&g_lptim_lock, flags);
  pwmerr("ERROR: LPTIM2 PWM update handshake timeout\n");
  return ret;
}

/****************************************************************************
 * Name: stm32n6_lppwm_stop
 ****************************************************************************/

static int stm32n6_lppwm_stop(struct pwm_lowerhalf_s *dev)
{
  struct stm32n6_lptim_pwm_s *priv = (struct stm32n6_lptim_pwm_s *)dev;
  irqstate_t flags;

  DEBUGASSERT(priv != NULL);

  flags = spin_lock_irqsave(&g_lptim_lock);

  /* Clearing ENABLE stops and resets the counter and drops the CH1 output */

  putreg32(0, priv->base + STM32_LPTIM_CR_OFFSET);
  putreg32(0, priv->base + STM32_LPTIM_CCMR1_OFFSET);

  spin_unlock_irqrestore(&g_lptim_lock, flags);
  return OK;
}

#endif /* CONFIG_STM32_LPTIM2_PWM */

#ifdef CONFIG_STM32_LPTIM_STOPWAKE

/****************************************************************************
 * Name: stm32n6_lptim_stopwake_isr
 *
 * Description:
 *   Wakeup interrupt for the Stop-mode self-test.  Runs on LPTIM3's global
 *   IRQ after the auto-reload match brings the CPU back from Stop mode.
 *   Acknowledges the LPTIM match and the EXTI line-55 pending bits, and
 *   records that LPTIM3 was the waker.
 *
 ****************************************************************************/

static volatile bool g_lptim_stopwake_woke;

static int stm32n6_lptim_stopwake_isr(int irq, void *context, void *arg)
{
  UNUSED(irq);
  UNUSED(context);
  UNUSED(arg);

  /* Acknowledge the auto-reload match and the EXTI wakeup pending bits so
   * the line does not immediately re-fire after we return.
   */

  putreg32(LPTIM_ICR_ARRMCF, STM32_LPTIM3_BASE + STM32_LPTIM_ICR_OFFSET);
  putreg32(EXTI_IMR2_LPTIM3, STM32_EXTI_RPR2);
  putreg32(EXTI_IMR2_LPTIM3, STM32_EXTI_FPR2);

  g_lptim_stopwake_woke = true;
  return OK;
}

#endif /* CONFIG_STM32_LPTIM_STOPWAKE */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_STM32_LPTIM_STOPWAKE

/****************************************************************************
 * Name: stm32n6_lptim_stopwake
 *
 * Description:
 *   Stop-mode wakeup self-test (ADR-028).  Arms LPTIM3 as a one-shot on the
 *   LSI, unmasks its EXTI line so it can wake the core, puts the CPU into
 *   Stop mode, and returns once the LPTIM match interrupt wakes it back up.
 *
 *   This proves the last differentiating LPTIM capability: the counter keeps
 *   running from the LSI while the CPU clock is gated in Stop mode, and its
 *   interrupt wakes the core.  On this chip an LPTIM wakes the CPU from Stop
 *   only if its EXTI IMR2 line is unmasked -- enabling the NVIC IRQ alone is
 *   not enough.
 *
 *   The test is destructive to LPTIM3 (it reprograms the peripheral from
 *   scratch and leaves it disabled), so it must not run concurrently with
 *   the /dev/timer3 timer driver.
 *
 * Input Parameters:
 *   ms       - Requested Stop duration in milliseconds (<= ~2048 ms, the
 *              16-bit LSI reload limit).
 *   stopf    - If non-NULL, receives true if PWR_CPUCR.STOPF confirmed Stop
 *              mode was genuinely entered.
 *   elapsed  - If non-NULL, receives the RTC seconds elapsed across the Stop
 *              (0 when CONFIG_RTC is absent).
 *
 * Returned Value:
 *   OK if Stop mode was entered (STOPF set) and LPTIM3 woke the core;
 *   a negated errno value otherwise.
 *
 ****************************************************************************/

int stm32n6_lptim_stopwake(unsigned int ms, bool *stopf,
                           unsigned int *elapsed)
{
  uint32_t ticks;
  uint32_t before = 0;
  uint32_t after = 0;
  bool entered;
  int i;
  int ret;

  /* One LPTIM tick is one LSI period; the 16-bit reload caps the interval */

  ticks = ((uint64_t)ms * STM32N6_LPTIM_LSI_FREQ) / 1000ull;
  if (ticks == 0 || ticks > STM32N6_LPTIM_MAXTICKS)
    {
      return -EINVAL;
    }

  /* Bring up LSI + LPTIM3 kernel clock + APB4 gate (with Sleep keep-alive) */

  ret = stm32n6_lptim_clk_bringup(STM32_RCC_APB4ENR1,
                                  RCC_APB4ENR1_LPTIM3EN,
                                  STM32_RCC_APB4LPENR1,
                                  RCC_APB4LPENR1_LPTIM3LPEN,
                                  RCC_CCIPR12_LPTIM3SEL_MASK,
                                  RCC_CCIPR12_LPTIM3SEL_LSI);
  if (ret < 0)
    {
      tmrerr("ERROR: LPTIM3 LSI clock enable timeout\n");
      return ret;
    }

  g_lptim_stopwake_woke = false;

  /* Configure while disabled: internal clock, prescaler /1, ARR preload,
   * and arm the auto-reload-match interrupt.
   */

  putreg32(0, STM32_LPTIM3_BASE + STM32_LPTIM_CR_OFFSET);
  putreg32(LPTIM_CFGR_PRELOAD, STM32_LPTIM3_BASE + STM32_LPTIM_CFGR_OFFSET);
  putreg32(LPTIM_DIER_ARRMIE, STM32_LPTIM3_BASE + STM32_LPTIM_DIER_OFFSET);

  /* Enable; ARR is writable only once enabled, then wait for ARROK */

  putreg32(LPTIM_CR_ENABLE, STM32_LPTIM3_BASE + STM32_LPTIM_CR_OFFSET);

  putreg32(LPTIM_ICR_ARROKCF, STM32_LPTIM3_BASE + STM32_LPTIM_ICR_OFFSET);
  putreg32(ticks - 1, STM32_LPTIM3_BASE + STM32_LPTIM_ARR_OFFSET);

  ret = -ETIMEDOUT;
  for (i = 0; i < STM32N6_LPTIM_TIMEOUT_US; i++)
    {
      if ((getreg32(STM32_LPTIM3_BASE + STM32_LPTIM_ISR_OFFSET) &
           LPTIM_ISR_ARROK) != 0)
        {
          ret = OK;
          break;
        }

      up_udelay(1);
    }

  if (ret < 0)
    {
      putreg32(0, STM32_LPTIM3_BASE + STM32_LPTIM_CR_OFFSET);
      tmrerr("ERROR: LPTIM3 ARR update (ARROK) timeout\n");
      return ret;
    }

  putreg32(LPTIM_ICR_ARROKCF, STM32_LPTIM3_BASE + STM32_LPTIM_ICR_OFFSET);
  putreg32(LPTIM_ICR_ARRMCF, STM32_LPTIM3_BASE + STM32_LPTIM_ICR_OFFSET);

  /* Route the wakeup: attach + enable the NVIC IRQ and unmask the EXTI
   * line.  Both are required to wake the CPU from Stop mode.
   */

  irq_attach(STM32_IRQ_LPTIM3, stm32n6_lptim_stopwake_isr, NULL);
  up_enable_irq(STM32_IRQ_LPTIM3);
  modifyreg32(STM32_EXTI_IMR2, 0, EXTI_IMR2_LPTIM3);

#ifdef CONFIG_RTC
  before = (uint32_t)time(NULL);
#endif

  /* Fire the one-shot and drop into Stop mode.  The LPTIM keeps counting on
   * the LSI while the CPU clock is gated; the match interrupt wakes us.
   */

  modifyreg32(STM32_LPTIM3_BASE + STM32_LPTIM_CR_OFFSET, 0,
              LPTIM_CR_SNGSTRT);

  entered = stm32n6_pwr_enter_stop();

#ifdef CONFIG_RTC
  after = (uint32_t)time(NULL);
#endif

  /* Tear down: mask the EXTI line, disable + detach the IRQ, stop LPTIM3 */

  modifyreg32(STM32_EXTI_IMR2, EXTI_IMR2_LPTIM3, 0);
  up_disable_irq(STM32_IRQ_LPTIM3);
  irq_detach(STM32_IRQ_LPTIM3);

  putreg32(0, STM32_LPTIM3_BASE + STM32_LPTIM_CR_OFFSET);
  putreg32(0, STM32_LPTIM3_BASE + STM32_LPTIM_DIER_OFFSET);
  putreg32(LPTIM_ICR_ARRMCF, STM32_LPTIM3_BASE + STM32_LPTIM_ICR_OFFSET);

  if (stopf != NULL)
    {
      *stopf = entered;
    }

  if (elapsed != NULL)
    {
      *elapsed = (after >= before) ? (after - before) : 0;
    }

  /* PASS requires both hardware witnesses: Stop was truly entered (STOPF)
   * and LPTIM3 is what woke the core (ISR ran).
   */

  if (!entered || !g_lptim_stopwake_woke)
    {
      return -EIO;
    }

  return OK;
}

#endif /* CONFIG_STM32_LPTIM_STOPWAKE */

/****************************************************************************
 * Name: stm32n6_lptim_initialize
 *
 * Description:
 *   Bind a low-power timer to a character device and register it.
 *
 * Input Parameters:
 *   devpath - Character device path (e.g. "/dev/timer1").
 *   timer   - LPTIM peripheral number (1 on APB1; 2-5 on APB4).
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_lptim_initialize(const char *devpath, int timer)
{
  struct stm32n6_lptim_lowerhalf_s *priv;
  void *handle;

  DEBUGASSERT(devpath != NULL);

  switch (timer)
    {
#ifdef CONFIG_STM32_LPTIM1
      case 1:
        priv = &g_lptim1_lowerhalf;
        break;
#endif

#ifdef CONFIG_STM32_LPTIM2
      case 2:
        priv = &g_lptim2_lowerhalf;
        break;
#endif

#ifdef CONFIG_STM32_LPTIM3
      case 3:
        priv = &g_lptim3_lowerhalf;
        break;
#endif

#ifdef CONFIG_STM32_LPTIM4
      case 4:
        priv = &g_lptim4_lowerhalf;
        break;
#endif

#ifdef CONFIG_STM32_LPTIM5
      case 5:
        priv = &g_lptim5_lowerhalf;
        break;
#endif

      default:
        return -ENODEV;
    }

  /* Attach the match ISR (kept disabled at the NVIC until start) */

  irq_attach(priv->irq, stm32n6_lptim_interrupt, priv);

  handle = timer_register(devpath, (struct timer_lowerhalf_s *)priv);
  if (handle == NULL)
    {
      irq_detach(priv->irq);
      return -EEXIST;
    }

  return OK;
}

#ifdef CONFIG_STM32_LPTIM2_PWM

/****************************************************************************
 * Name: stm32n6_lppwm_initialize
 *
 * Description:
 *   Register LPTIM2 as a PWM character device driving LPTIM2_CH1 from LSI.
 *
 ****************************************************************************/

int stm32n6_lppwm_initialize(const char *devpath)
{
  struct stm32n6_lptim_pwm_s *priv = &g_lptim2_pwm;

  DEBUGASSERT(devpath != NULL);

  /* The hardware drives the CH1 waveform autonomously; no IRQ is attached */

  return pwm_register(devpath, (struct pwm_lowerhalf_s *)priv);
}
#endif /* CONFIG_STM32_LPTIM2_PWM */
