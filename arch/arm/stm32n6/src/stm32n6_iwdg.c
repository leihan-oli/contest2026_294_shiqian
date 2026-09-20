/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_iwdg.c
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

/* ADR-015: Independent Watchdog (IWDG) lower-half driver.
 *
 * Delivers the IWDG function of ADR-015 through the NuttX watchdog
 * character framework (/dev/watchdog0).  The IWDG is an LSI-clocked
 * (~32 kHz) down-counter that resets the MCU if it is not reloaded
 * ("fed") before it reaches zero.  Because it runs from the LSI it keeps
 * running even if the main clock tree fails, which is exactly the
 * hardware-hang safety net the watchdog is meant to provide.
 *
 * The prescaler (PR) and reload (RLR) registers are write-protected; a
 * 0x5555 key must be written to KR first.  After a PR/RLR write the SR
 * PVU/RVU busy bits stay set until the value propagates across the LSI
 * clock domain, so every setup polls them with a bounded timeout (never
 * a dead wait -- Embedded Programming Rule 2).
 *
 * The counter cannot be stopped once started (there is no disable key),
 * so stop() returns -ENOSYS exactly like the STM32H7 reference.
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

#include "arm_internal.h"
#include "chip.h"
#include "stm32n6_iwdg.h"
#include "hardware/stm32_iwdg.h"
#include "hardware/stm32_rcc.h"

#ifdef CONFIG_STM32_IWDG

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* LSI drives the IWDG at a nominal 32 kHz.  The smallest prescaler (/4)
 * gives the finest resolution; the largest (/256) gives the longest
 * timeout.  The maximum timeout (ms) is 1000 * RLR_MAX / (LSI / 256).
 */

#define IWDG_LSI_FREQ        32000
#define IWDG_FMIN            (IWDG_LSI_FREQ / 256)
#define IWDG_MAXTIMEOUT      (1000 * IWDG_RLR_MAX / IWDG_FMIN)

/* Bounded busy-poll budget for the PVU/RVU propagation handshake */

#define IWDG_SR_TIMEOUT_US   10000

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_iwdg_lowerhalf_s
{
  const struct watchdog_ops_s *ops;       /* Lower-half ops (must be 1st) */
  uint32_t                     timeout;   /* Actual timeout (ms) */
  uint32_t                     lastreset; /* Tick count at last feed */
  bool                         started;   /* True once the WDT is running */
  uint8_t                      prescaler; /* PR field value (0..6) */
  uint16_t                     reload;    /* RLR field value */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32n6_iwdg_start(struct watchdog_lowerhalf_s *lower);
static int stm32n6_iwdg_stop(struct watchdog_lowerhalf_s *lower);
static int stm32n6_iwdg_keepalive(struct watchdog_lowerhalf_s *lower);
static int stm32n6_iwdg_getstatus(struct watchdog_lowerhalf_s *lower,
                                  struct watchdog_status_s *status);
static int stm32n6_iwdg_settimeout(struct watchdog_lowerhalf_s *lower,
                                   uint32_t timeout);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct watchdog_ops_s g_stm32n6_iwdg_ops =
{
  .start      = stm32n6_iwdg_start,
  .stop       = stm32n6_iwdg_stop,
  .keepalive  = stm32n6_iwdg_keepalive,
  .getstatus  = stm32n6_iwdg_getstatus,
  .settimeout = stm32n6_iwdg_settimeout,
  .capture    = NULL,
  .ioctl      = NULL,
};

static struct stm32n6_iwdg_lowerhalf_s g_iwdg_lowerhalf =
{
  .ops = &g_stm32n6_iwdg_ops,
};

static spinlock_t g_iwdg_lock = SP_UNLOCKED;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_iwdg_wait_sr
 *
 * Description:
 *   Poll the IWDG status register until the given busy bits clear, bounded
 *   by IWDG_SR_TIMEOUT_US.  The PVU/RVU bits stay set while a PR/RLR write
 *   propagates across the LSI clock domain.
 *
 ****************************************************************************/

static int stm32n6_iwdg_wait_sr(uint32_t bits)
{
  int i;

  for (i = 0; i < IWDG_SR_TIMEOUT_US; i++)
    {
      if ((getreg32(STM32_IWDG_SR) & bits) == 0)
        {
          return OK;
        }

      up_udelay(1);
    }

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: stm32n6_iwdg_setprescaler
 *
 * Description:
 *   Unlock and program the prescaler and reload registers, then reload the
 *   counter.  Must be called before starting the watchdog.
 *
 ****************************************************************************/

static int stm32n6_iwdg_setprescaler(struct stm32n6_iwdg_lowerhalf_s *priv)
{
  int ret;

  /* Enable write access to PR/RLR */

  putreg32(IWDG_KR_KEY_ACCESS, STM32_IWDG_KR);

  /* The PR/RLR writes only take once the previous value has propagated */

  ret = stm32n6_iwdg_wait_sr(IWDG_SR_PVU | IWDG_SR_RVU);
  if (ret < 0)
    {
      return ret;
    }

  putreg32(priv->prescaler, STM32_IWDG_PR);
  putreg32(priv->reload, STM32_IWDG_RLR);

  /* Wait for both writes to propagate before locking again */

  ret = stm32n6_iwdg_wait_sr(IWDG_SR_PVU | IWDG_SR_RVU);
  if (ret < 0)
    {
      return ret;
    }

  /* Reload the counter (this also re-locks PR/RLR write access) */

  putreg32(IWDG_KR_KEY_RELOAD, STM32_IWDG_KR);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_iwdg_start
 ****************************************************************************/

static int stm32n6_iwdg_start(struct watchdog_lowerhalf_s *lower)
{
  struct stm32n6_iwdg_lowerhalf_s *priv =
    (struct stm32n6_iwdg_lowerhalf_s *)lower;
  irqstate_t flags;
  int ret = OK;

  DEBUGASSERT(priv != NULL);

  if (priv->started)
    {
      return OK;
    }

  flags = spin_lock_irqsave(&g_iwdg_lock);

  /* Program prescaler/reload for the selected timeout, then start.  The
   * IWDG enable key also turns on the LSI automatically in hardware.
   */

  ret = stm32n6_iwdg_setprescaler(priv);
  if (ret < 0)
    {
      spin_unlock_irqrestore(&g_iwdg_lock, flags);
      wderr("ERROR: IWDG prescaler setup timed out\n");
      return ret;
    }

  putreg32(IWDG_KR_KEY_START, STM32_IWDG_KR);
  priv->lastreset = clock_systime_ticks();
  priv->started   = true;

  spin_unlock_irqrestore(&g_iwdg_lock, flags);
  return OK;
}

/****************************************************************************
 * Name: stm32n6_iwdg_stop
 ****************************************************************************/

static int stm32n6_iwdg_stop(struct watchdog_lowerhalf_s *lower)
{
  /* The IWDG cannot be disabled once started -- there is no stop key */

  UNUSED(lower);
  return -ENOSYS;
}

/****************************************************************************
 * Name: stm32n6_iwdg_keepalive
 ****************************************************************************/

static int stm32n6_iwdg_keepalive(struct watchdog_lowerhalf_s *lower)
{
  struct stm32n6_iwdg_lowerhalf_s *priv =
    (struct stm32n6_iwdg_lowerhalf_s *)lower;
  irqstate_t flags;

  DEBUGASSERT(priv != NULL);

  flags = spin_lock_irqsave(&g_iwdg_lock);
  putreg32(IWDG_KR_KEY_RELOAD, STM32_IWDG_KR);
  priv->lastreset = clock_systime_ticks();
  spin_unlock_irqrestore(&g_iwdg_lock, flags);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_iwdg_getstatus
 ****************************************************************************/

static int stm32n6_iwdg_getstatus(struct watchdog_lowerhalf_s *lower,
                                  struct watchdog_status_s *status)
{
  struct stm32n6_iwdg_lowerhalf_s *priv =
    (struct stm32n6_iwdg_lowerhalf_s *)lower;
  uint32_t elapsed;
  uint32_t ticks;

  DEBUGASSERT(priv != NULL && status != NULL);

  status->flags = WDFLAGS_RESET;
  if (priv->started)
    {
      status->flags |= WDFLAGS_ACTIVE;
    }

  status->timeout = priv->timeout;

  /* Approximate the time left from the tick delta since the last feed */

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
 * Name: stm32n6_iwdg_settimeout
 ****************************************************************************/

static int stm32n6_iwdg_settimeout(struct watchdog_lowerhalf_s *lower,
                                   uint32_t timeout)
{
  struct stm32n6_iwdg_lowerhalf_s *priv =
    (struct stm32n6_iwdg_lowerhalf_s *)lower;
  uint32_t fiwdg;
  uint64_t reload;
  int prescaler;
  int shift;

  DEBUGASSERT(priv != NULL);

  if (timeout < 1 || timeout > IWDG_MAXTIMEOUT)
    {
      wderr("ERROR: timeout=%" PRIu32 " out of range [1,%d]\n",
            timeout, IWDG_MAXTIMEOUT);
      return -ERANGE;
    }

  /* The PR/RLR pair can only be programmed reliably before the counter is
   * started, so refuse a change once running (matches the H7 reference).
   */

  if (priv->started)
    {
      wdwarn("WARNING: IWDG already started; timeout is fixed\n");
      return -EBUSY;
    }

  /* Pick the smallest prescaler whose reload fits in 12 bits.  Divider is
   * 4 << prescaler, i.e. the counter clock is LSI >> (prescaler + 2).
   */

  for (prescaler = 0; ; prescaler++)
    {
      shift  = prescaler + 2;
      fiwdg  = IWDG_LSI_FREQ >> shift;
      reload = (uint64_t)fiwdg * (uint64_t)timeout / 1000;

      if (reload <= IWDG_RLR_MAX || prescaler == IWDG_PR_MAX)
        {
          break;
        }
    }

  if (reload > IWDG_RLR_MAX)
    {
      reload = IWDG_RLR_MAX;
    }

  /* Report back the achievable timeout for this prescaler/reload pair */

  priv->timeout   = (1000 * (uint32_t)reload) / fiwdg;
  priv->prescaler = (uint8_t)prescaler;
  priv->reload    = (uint16_t)reload;

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_iwdg_initialize
 *
 * Description:
 *   Register the IWDG as a watchdog character device.  The watchdog is
 *   left stopped; the caller starts it via the WDIOC_START ioctl.
 *
 * Input Parameters:
 *   devpath - Character device path (e.g. "/dev/watchdog0").
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_iwdg_initialize(const char *devpath)
{
  struct stm32n6_iwdg_lowerhalf_s *priv = &g_iwdg_lowerhalf;
  void *handle;

  DEBUGASSERT(devpath != NULL);

  priv->started = false;

  /* Preload an arbitrary maximum timeout so a bare WDIOC_START (without a
   * prior WDIOC_SETTIMEOUT) still has a valid prescaler/reload to apply.
   */

  stm32n6_iwdg_settimeout((struct watchdog_lowerhalf_s *)priv,
                          IWDG_MAXTIMEOUT);

  handle = watchdog_register(devpath,
                             (struct watchdog_lowerhalf_s *)priv);
  if (handle == NULL)
    {
      wderr("ERROR: watchdog_register(%s) failed\n", devpath);
      return -EEXIST;
    }

  return OK;
}

#endif /* CONFIG_STM32_IWDG */
