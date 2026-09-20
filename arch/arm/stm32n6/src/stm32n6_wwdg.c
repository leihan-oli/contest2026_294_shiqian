/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_wwdg.c
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

/* ADR-015: Window Watchdog (WWDG) lower-half driver.
 *
 * Complements the IWDG (stm32n6_iwdg.c) with the second STM32N6 watchdog
 * through the NuttX watchdog framework (/dev/watchdog1).  The WWDG is
 * APB1-clocked and, unlike the IWDG, provides an Early-Wakeup Interrupt
 * (EWI) that fires one tick before the reset.  That interrupt is what lets
 * this driver implement the optional watchdog capture() op: a user handler
 * runs from the EWI just ahead of the reset, exactly the "warn before the
 * dog bites" hook the drivertest_watchdog capture path exercises.
 *
 * The 7-bit down-counter resets the MCU when it rolls under 0x3f (the T6
 * bit clears), so the usable counter span is 0x40..0x7f (64 ticks).  The
 * window register W is left at 0x7f so a refresh is accepted at any time
 * (no early-refresh reset), turning the WWDG into a plain timeout dog for
 * the generic framework.  Timeout is tuned only through the WDGTB timer
 * base prescaler.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/clock.h>
#include <nuttx/irq.h>
#include <nuttx/spinlock.h>
#include <nuttx/timers/watchdog.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "chip.h"
#include "stm32n6_wwdg.h"
#include "hardware/stm32_wwdg.h"
#include "hardware/stm32_rcc.h"

#ifdef CONFIG_STM32_WWDG

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The WWDG counter clock is PCLK1 / 4096 / (1 << WDGTB).  One counter tick
 * is therefore 4096 * (1 << WDGTB) / PCLK1 seconds; the full 64-count span
 * gives the maximum timeout at a given WDGTB.
 */

#define WWDG_PCLK            STM32_PCLK1_FREQUENCY
#define WWDG_BASE_DIV        4096
#define WWDG_COUNT_SPAN      64          /* 0x40..0x7f usable counts */
#define WWDG_WDGTB_MAX       7           /* 3-bit timer-base prescaler */

/* Maximum timeout (ms) with the largest timer base */

#define WWDG_TICK_US(tb)     (((uint64_t)WWDG_BASE_DIV * (1u << (tb)) * \
                              1000000ull) / WWDG_PCLK)
#define WWDG_MAXTIMEOUT      ((uint32_t)((WWDG_TICK_US(WWDG_WDGTB_MAX) * \
                              WWDG_COUNT_SPAN) / 1000))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_wwdg_lowerhalf_s
{
  const struct watchdog_ops_s *ops;       /* Lower-half ops (must be 1st) */
  xcpt_t                       handler;   /* Captured EWI handler, or NULL */
  uint32_t                     timeout;   /* Actual timeout (ms) */
  uint32_t                     lastreset; /* Tick count at last feed */
  uint8_t                      wdgtb;     /* Selected timer-base prescaler */
  bool                         started;   /* True once the WDT is running */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int    stm32n6_wwdg_start(struct watchdog_lowerhalf_s *lower);
static int    stm32n6_wwdg_stop(struct watchdog_lowerhalf_s *lower);
static int    stm32n6_wwdg_keepalive(struct watchdog_lowerhalf_s *lower);
static int    stm32n6_wwdg_getstatus(struct watchdog_lowerhalf_s *lower,
                                     struct watchdog_status_s *status);
static int    stm32n6_wwdg_settimeout(struct watchdog_lowerhalf_s *lower,
                                      uint32_t timeout);
static xcpt_t stm32n6_wwdg_capture(struct watchdog_lowerhalf_s *lower,
                                   xcpt_t handler);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct watchdog_ops_s g_stm32n6_wwdg_ops =
{
  .start      = stm32n6_wwdg_start,
  .stop       = stm32n6_wwdg_stop,
  .keepalive  = stm32n6_wwdg_keepalive,
  .getstatus  = stm32n6_wwdg_getstatus,
  .settimeout = stm32n6_wwdg_settimeout,
  .capture    = stm32n6_wwdg_capture,
  .ioctl      = NULL,
};

static struct stm32n6_wwdg_lowerhalf_s g_wwdg_lowerhalf =
{
  .ops   = &g_stm32n6_wwdg_ops,
  .wdgtb = WWDG_WDGTB_MAX,
};

static spinlock_t g_wwdg_lock = SP_UNLOCKED;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_wwdg_enableclk
 *
 * Description:
 *   Open the APB1 peripheral clock gate feeding the WWDG.  Without this a
 *   register write is silently dropped and reads return zero.
 *
 ****************************************************************************/

static void stm32n6_wwdg_enableclk(void)
{
  uint32_t regval;

  regval  = getreg32(STM32_RCC_APB1ENR1);
  regval |= RCC_APB1ENR1_WWDGEN;
  putreg32(regval, STM32_RCC_APB1ENR1);
}

/****************************************************************************
 * Name: stm32n6_wwdg_interrupt
 *
 * Description:
 *   Early-wakeup interrupt handler.  Acknowledges the flag and, if a
 *   capture handler is registered, forwards to it just before the pending
 *   reset.  With no handler the flag is cleared and the reset proceeds.
 *
 ****************************************************************************/

static int stm32n6_wwdg_interrupt(int irq, void *context, void *arg)
{
  struct stm32n6_wwdg_lowerhalf_s *priv =
    (struct stm32n6_wwdg_lowerhalf_s *)arg;

  DEBUGASSERT(priv != NULL);

  if ((getreg32(STM32_WWDG_SR) & WWDG_SR_EWIF) == 0)
    {
      return OK;
    }

  /* Acknowledge the early-wakeup flag (rc_w0: write 0 to clear) */

  putreg32(0, STM32_WWDG_SR);

  if (priv->handler != NULL)
    {
      priv->handler(irq, context, arg);
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_wwdg_start
 ****************************************************************************/

static int stm32n6_wwdg_start(struct watchdog_lowerhalf_s *lower)
{
  struct stm32n6_wwdg_lowerhalf_s *priv =
    (struct stm32n6_wwdg_lowerhalf_s *)lower;
  irqstate_t flags;
  uint32_t cfr;

  DEBUGASSERT(priv != NULL);

  if (priv->started)
    {
      return OK;
    }

  flags = spin_lock_irqsave(&g_wwdg_lock);

  /* Program the timer base and open the refresh window fully (W = max so a
   * feed is accepted at any time).  Enable the early-wakeup interrupt so
   * the optional capture handler can run just before the reset.
   */

  cfr  = (uint32_t)priv->wdgtb << WWDG_CFR_WDGTB_SHIFT;
  cfr |= WWDG_CR_T_MAX;                 /* W = 0x7f: window disabled */
  cfr |= WWDG_CFR_EWI;
  putreg32(cfr, STM32_WWDG_CFR);

  /* Load the counter to its maximum and activate the watchdog (WDGA is a
   * write-once-until-reset bit; the counter now runs).
   */

  putreg32(WWDG_CR_WDGA | WWDG_CR_T_MAX, STM32_WWDG_CR);

  priv->lastreset = clock_systime_ticks();
  priv->started   = true;

  spin_unlock_irqrestore(&g_wwdg_lock, flags);

  up_enable_irq(STM32_IRQ_WWDG);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_wwdg_stop
 ****************************************************************************/

static int stm32n6_wwdg_stop(struct watchdog_lowerhalf_s *lower)
{
  /* Like the IWDG, WWDG activation (WDGA) can only be cleared by a system
   * reset, so the running counter cannot be stopped from software.
   */

  UNUSED(lower);
  return -ENOSYS;
}

/****************************************************************************
 * Name: stm32n6_wwdg_keepalive
 ****************************************************************************/

static int stm32n6_wwdg_keepalive(struct watchdog_lowerhalf_s *lower)
{
  struct stm32n6_wwdg_lowerhalf_s *priv =
    (struct stm32n6_wwdg_lowerhalf_s *)lower;
  irqstate_t flags;

  DEBUGASSERT(priv != NULL);

  /* Reload the counter to its maximum.  WDGA stays set; only T is rewritten
   * (writing WDGA again is harmless while already active).
   */

  flags = spin_lock_irqsave(&g_wwdg_lock);
  putreg32(WWDG_CR_WDGA | WWDG_CR_T_MAX, STM32_WWDG_CR);
  priv->lastreset = clock_systime_ticks();
  spin_unlock_irqrestore(&g_wwdg_lock, flags);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_wwdg_getstatus
 ****************************************************************************/

static int stm32n6_wwdg_getstatus(struct watchdog_lowerhalf_s *lower,
                                  struct watchdog_status_s *status)
{
  struct stm32n6_wwdg_lowerhalf_s *priv =
    (struct stm32n6_wwdg_lowerhalf_s *)lower;
  uint32_t elapsed;
  uint32_t ticks;

  DEBUGASSERT(priv != NULL && status != NULL);

  status->flags = WDFLAGS_RESET;
  if (priv->started)
    {
      status->flags |= WDFLAGS_ACTIVE;
    }

  if (priv->handler != NULL)
    {
      status->flags |= WDFLAGS_CAPTURE;
    }

  status->timeout = priv->timeout;

  ticks   = clock_systime_ticks() - priv->lastreset;
  elapsed = TICK2MSEC(ticks);
  if (elapsed > priv->timeout)
    {
      elapsed = priv->timeout;
    }

  status->timeleft = priv->timeout - elapsed;
  return OK;
}

/****************************************************************************
 * Name: stm32n6_wwdg_settimeout
 ****************************************************************************/

static int stm32n6_wwdg_settimeout(struct watchdog_lowerhalf_s *lower,
                                   uint32_t timeout)
{
  struct stm32n6_wwdg_lowerhalf_s *priv =
    (struct stm32n6_wwdg_lowerhalf_s *)lower;
  int tb;

  DEBUGASSERT(priv != NULL);

  if (timeout < 1 || timeout > WWDG_MAXTIMEOUT)
    {
      wderr("ERROR: timeout=%" PRIu32 " out of range [1,%" PRIu32 "]\n",
            timeout, (uint32_t)WWDG_MAXTIMEOUT);
      return -ERANGE;
    }

  if (priv->started)
    {
      wdwarn("WARNING: WWDG already started; timeout is fixed\n");
      return -EBUSY;
    }

  /* Pick the smallest timer base whose full-span timeout covers the
   * request, then record the achievable timeout for that base.
   */

  for (tb = 0; tb < WWDG_WDGTB_MAX; tb++)
    {
      uint32_t span = (uint32_t)((WWDG_TICK_US(tb) * WWDG_COUNT_SPAN)
                                 / 1000);
      if (span >= timeout)
        {
          break;
        }
    }

  priv->wdgtb   = (uint8_t)tb;
  priv->timeout = (uint32_t)((WWDG_TICK_US(tb) * WWDG_COUNT_SPAN) / 1000);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_wwdg_capture
 ****************************************************************************/

static xcpt_t stm32n6_wwdg_capture(struct watchdog_lowerhalf_s *lower,
                                   xcpt_t handler)
{
  struct stm32n6_wwdg_lowerhalf_s *priv =
    (struct stm32n6_wwdg_lowerhalf_s *)lower;
  irqstate_t flags;
  xcpt_t oldhandler;

  DEBUGASSERT(priv != NULL);

  flags = spin_lock_irqsave(&g_wwdg_lock);
  oldhandler    = priv->handler;
  priv->handler = handler;
  spin_unlock_irqrestore(&g_wwdg_lock, flags);

  return oldhandler;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_wwdg_initialize
 *
 * Description:
 *   Register the WWDG as a watchdog character device.  The watchdog is
 *   left stopped; the caller starts it via the WDIOC_START ioctl.
 *
 * Input Parameters:
 *   devpath - Character device path (e.g. "/dev/watchdog1").
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_wwdg_initialize(const char *devpath)
{
  struct stm32n6_wwdg_lowerhalf_s *priv = &g_wwdg_lowerhalf;
  void *handle;

  DEBUGASSERT(devpath != NULL);

  priv->started = false;
  priv->handler = NULL;

  stm32n6_wwdg_enableclk();

  /* Attach the early-wakeup ISR (kept masked at the NVIC until start) */

  irq_attach(STM32_IRQ_WWDG, stm32n6_wwdg_interrupt, priv);

  /* Preload the maximum achievable timeout so a bare WDIOC_START has a
   * valid timer-base to program.
   */

  stm32n6_wwdg_settimeout((struct watchdog_lowerhalf_s *)priv,
                          WWDG_MAXTIMEOUT);

  handle = watchdog_register(devpath,
                             (struct watchdog_lowerhalf_s *)priv);
  if (handle == NULL)
    {
      irq_detach(STM32_IRQ_WWDG);
      wderr("ERROR: watchdog_register(%s) failed\n", devpath);
      return -EEXIST;
    }

  return OK;
}

#endif /* CONFIG_STM32_WWDG */
