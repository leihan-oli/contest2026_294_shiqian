/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_rtc.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <debug.h>

#include <time.h>

#include <nuttx/arch.h>
#include <nuttx/clock.h>

#include "arm_internal.h"
#include "stm32n6_rtc.h"
#include "hardware/stm32_pwr.h"
#include "hardware/stm32_rcc.h"
#include "hardware/stm32_rtc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Timeout for init mode entry (approximate) */

#define INITMODE_TIMEOUT  0x00010000

/* BCD conversion macros */

#define BCD2BIN(bcd) ((((bcd) >> 4) * 10) + ((bcd) & 0x0f))
#define BIN2BCD(bin) ((((bin) / 10) << 4) | ((bin) % 10))

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_rtc_initialized = false;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: rtc_wpr_unlock
 *
 * Description:
 *   Disable RTC write protection.
 *
 ****************************************************************************/

static void rtc_wpr_unlock(void)
{
  /* Unlock sequence: write 0xca then 0x53 to WPR */

  putreg32(0xca, STM32N6_RTC_WPR);
  putreg32(0x53, STM32N6_RTC_WPR);
}

/****************************************************************************
 * Name: rtc_wpr_lock
 *
 * Description:
 *   Re-enable RTC write protection.
 *
 ****************************************************************************/

static void rtc_wpr_lock(void)
{
  /* Writing any value other than the unlock keys re-enables protection */

  putreg32(0xff, STM32N6_RTC_WPR);
}

/****************************************************************************
 * Name: rtc_enter_init
 *
 * Description:
 *   Enter RTC initialization mode.
 *
 * Returned Value:
 *   Zero (OK) on success; -ETIMEDOUT on failure
 *
 ****************************************************************************/

static int rtc_enter_init(void)
{
  volatile uint32_t timeout;
  uint32_t regval;

  /* Check if already in init mode */

  regval = getreg32(STM32N6_RTC_ICSR);
  if ((regval & RTC_ICSR_INITF) != 0)
    {
      return OK;
    }

  /* Set INIT bit to enter init mode */

  putreg32(RTC_ICSR_INIT, STM32N6_RTC_ICSR);

  /* Wait for INITF to be set */

  for (timeout = 0; timeout < INITMODE_TIMEOUT; timeout++)
    {
      regval = getreg32(STM32N6_RTC_ICSR);
      if ((regval & RTC_ICSR_INITF) != 0)
        {
          return OK;
        }
    }

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: rtc_exit_init
 *
 * Description:
 *   Exit RTC initialization mode.
 *
 ****************************************************************************/

static void rtc_exit_init(void)
{
  uint32_t regval;

  regval  = getreg32(STM32N6_RTC_ICSR);
  regval &= ~RTC_ICSR_INIT;
  putreg32(regval, STM32N6_RTC_ICSR);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_rtc_initialize
 *
 * Description:
 *   Initialize the RTC peripheral.  Called from board_late_initialize().
 *   Enables the RTC clock, configures prescaler for 1Hz tick from LSI.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno on failure
 *
 ****************************************************************************/

int stm32n6_rtc_initialize(void)
{
  uint32_t regval;
  uint32_t timeout;
  int ret;

  if (g_rtc_initialized)
    {
      return OK;
    }

  /* Enable RTC clock in RCC APB4ENR1 (CMSIS RCC_APB4ENR1_RTCEN) */

  regval  = getreg32(STM32_RCC_APB4ENR1);
  regval |= RCC_APB4ENR1_RTCEN;
  putreg32(regval, STM32_RCC_APB4ENR1);

  /* Configure the RTC kernel clock source.  RTCEN above only gates the
   * APB register-interface clock; without a running kernel clock the RTC
   * never enters init mode (INITF stays low).  Enable backup-domain write
   * access, start the LSI oscillator, and select it as the RTC source.
   */

  regval  = getreg32(STM32_PWR_DBPCR);
  regval |= PWR_DBPCR_DBP;
  putreg32(regval, STM32_PWR_DBPCR);

  /* Enable LSI (~32kHz) via the CSR atomic set alias, then wait for
   * RCC_SR.LSIRDY with a bounded timeout (never spin forever on HW).
   */

  putreg32(RCC_CR_LSION, STM32_RCC_CSR);
  for (timeout = 0; timeout < INITMODE_TIMEOUT; timeout++)
    {
      if ((getreg32(STM32_RCC_SR) & RCC_SR_LSIRDY) != 0)
        {
          break;
        }
    }

  if ((getreg32(STM32_RCC_SR) & RCC_SR_LSIRDY) == 0)
    {
      rtcerr("ERROR: LSI failed to start\n");
      return -ETIMEDOUT;
    }

  /* Select LSI as the RTC kernel clock (CCIPR7.RTCSEL = LSI) */

  regval  = getreg32(STM32_RCC_CCIPR7);
  regval &= ~RCC_CCIPR7_RTCSEL_MASK;
  regval |= RCC_CCIPR7_RTCSEL_LSI;
  putreg32(regval, STM32_RCC_CCIPR7);

  /* Unlock write protection */

  rtc_wpr_unlock();

  /* Check if RTC was already initialized by reading magic from BKP0R */

  regval = getreg32(STM32N6_RTC_BKP0R);
  if (regval == STM32N6_RTC_MAGIC ||
      regval == STM32N6_RTC_MAGIC_TIME_SET)
    {
      /* RTC already configured, just exit init mode if stuck */

      rtc_exit_init();
      rtc_wpr_lock();
      g_rtc_initialized = true;
      rtcinfo("RTC: already initialized\n");
      return OK;
    }

  /* Cold boot: configure RTC from scratch */

  ret = rtc_enter_init();
  if (ret < 0)
    {
      rtcerr("ERROR: failed to enter init mode\n");
      rtc_wpr_lock();
      return ret;
    }

  /* Set 24-hour format (FMT = 0 in CR) */

  regval  = getreg32(STM32N6_RTC_CR);
  regval &= ~RTC_CR_FMT;
  putreg32(regval, STM32N6_RTC_CR);

  /* Configure prescaler for LSI ~32kHz:
   * PREDIV_A = 127, PREDIV_S = 249
   * 32000 / (128 * 250) = 1 Hz
   */

  putreg32((STM32N6_RTC_PREDIV_A_DEFAULT << RTC_PRER_PREDIV_A_SHIFT) |
           (STM32N6_RTC_PREDIV_S_DEFAULT << RTC_PRER_PREDIV_S_SHIFT),
           STM32N6_RTC_PRER);

  /* Set default time: 2026-01-01 00:00:00 (Thursday) */

  putreg32(0, STM32N6_RTC_TR);

  /* DR: Year=0x26, Month=0x01, Day=0x01, WDU=4 (Thursday) */

  putreg32((0x26 << RTC_DR_YU_SHIFT) |
           (0x01 << RTC_DR_MU_SHIFT) |
           (0x01 << RTC_DR_DU_SHIFT) |
           (4 << RTC_DR_WDU_SHIFT),
           STM32N6_RTC_DR);

  /* Exit init mode */

  rtc_exit_init();

  /* Mark RTC as initialized in backup register */

  putreg32(STM32N6_RTC_MAGIC, STM32N6_RTC_BKP0R);

  /* Re-enable write protection */

  rtc_wpr_lock();

  g_rtc_initialized = true;
  rtcinfo("RTC: initialized (cold boot)\n");

  return OK;
}

/****************************************************************************
 * Name: stm32n6_rtc_getdatetime
 *
 * Description:
 *   Get the current date and time from the RTC.
 *
 * Input Parameters:
 *   tp - The location to return the date/time value.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno on failure
 *
 ****************************************************************************/

int stm32n6_rtc_getdatetime(struct tm *tp)
{
  uint32_t dr;
  uint32_t tr;
  uint32_t tmp;

  DEBUGASSERT(tp != NULL);

  /* Sample time and date registers with consistency check.
   * Loop until DR reads the same before and after TR.
   */

  do
    {
      dr = getreg32(STM32N6_RTC_DR);
      tr = getreg32(STM32N6_RTC_TR);
    }
  while (dr != getreg32(STM32N6_RTC_DR));

  /* Convert TR (time register) BCD fields to struct tm */

  tmp = (tr & (RTC_TR_SU_MASK | RTC_TR_ST_MASK)) >> RTC_TR_SU_SHIFT;
  tp->tm_sec = BCD2BIN(tmp);

  tmp = (tr & (RTC_TR_MNU_MASK | RTC_TR_MNT_MASK)) >> RTC_TR_MNU_SHIFT;
  tp->tm_min = BCD2BIN(tmp);

  tmp = (tr & (RTC_TR_HU_MASK | RTC_TR_HT_MASK)) >> RTC_TR_HU_SHIFT;
  tp->tm_hour = BCD2BIN(tmp);

  /* Convert DR (date register) BCD fields to struct tm */

  tmp = (dr & (RTC_DR_DU_MASK | RTC_DR_DT_MASK)) >> RTC_DR_DU_SHIFT;
  tp->tm_mday = BCD2BIN(tmp);

  tmp = (dr & (RTC_DR_MU_MASK | RTC_DR_MT)) >> RTC_DR_MU_SHIFT;
  tp->tm_mon = BCD2BIN(tmp) - 1;

  tmp = (dr & (RTC_DR_YU_MASK | RTC_DR_YT_MASK)) >> RTC_DR_YU_SHIFT;
  tp->tm_year = BCD2BIN(tmp) + 100;

  tmp = (dr & RTC_DR_WDU_MASK) >> RTC_DR_WDU_SHIFT;
  tp->tm_wday = tmp % 7;

  tp->tm_yday = tp->tm_mday - 1 +
    clock_daysbeforemonth(tp->tm_mon,
                          clock_isleapyear(tp->tm_year + 1900));
  tp->tm_isdst = 0;

  return OK;
}

/****************************************************************************
 * Name: stm32n6_rtc_setdatetime
 *
 * Description:
 *   Set the RTC to the provided time.
 *
 * Input Parameters:
 *   tp - the time to use
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno on failure
 *
 ****************************************************************************/

int stm32n6_rtc_setdatetime(const struct tm *tp)
{
  uint32_t tr;
  uint32_t dr;
  int ret;

  DEBUGASSERT(tp != NULL);

  /* Convert struct tm fields to BCD for TR register */

  tr = (BIN2BCD(tp->tm_sec)  << RTC_TR_SU_SHIFT) |
       (BIN2BCD(tp->tm_min)  << RTC_TR_MNU_SHIFT) |
       (BIN2BCD(tp->tm_hour) << RTC_TR_HU_SHIFT);
  tr &= ~RTC_TR_RESERVED_BITS;

  /* Convert struct tm fields to BCD for DR register */

  dr = (BIN2BCD(tp->tm_mday) << RTC_DR_DU_SHIFT) |
       (BIN2BCD(tp->tm_mon + 1) << RTC_DR_MU_SHIFT) |
       ((tp->tm_wday == 0 ? 7 : (tp->tm_wday & 7)) << RTC_DR_WDU_SHIFT) |
       (BIN2BCD(tp->tm_year - 100) << RTC_DR_YU_SHIFT);
  dr &= ~RTC_DR_RESERVED_BITS;

  /* Unlock write protection */

  rtc_wpr_unlock();

  /* Enter init mode */

  ret = rtc_enter_init();
  if (ret == OK)
    {
      /* Write time and date registers */

      putreg32(tr, STM32N6_RTC_TR);
      putreg32(dr, STM32N6_RTC_DR);

      /* Exit init mode */

      rtc_exit_init();
    }

  /* Mark that time has been set */

  putreg32(STM32N6_RTC_MAGIC_TIME_SET, STM32N6_RTC_BKP0R);

  /* Re-enable write protection */

  rtc_wpr_lock();

  return ret;
}

/****************************************************************************
 * Name: stm32n6_rtc_havesettime
 *
 * Description:
 *   Check if RTC time has been previously set.
 *
 * Returned Value:
 *   Returns true if RTC date-time have been previously set.
 *
 ****************************************************************************/

bool stm32n6_rtc_havesettime(void)
{
  return getreg32(STM32N6_RTC_BKP0R) == STM32N6_RTC_MAGIC_TIME_SET;
}

/****************************************************************************
 * NuttX RTC framework bridge (CONFIG_RTC)
 *
 * The NuttX clock subsystem drives the RTC through the standard up_rtc_*
 * interface and the g_rtc_enabled flag.  These wrappers adapt that interface
 * onto the stm32n6_rtc_* primitives above.
 ****************************************************************************/

#ifdef CONFIG_RTC

/* Set true once the RTC has been successfully initialized */

volatile bool g_rtc_enabled = false;

/****************************************************************************
 * Name: up_rtc_initialize
 ****************************************************************************/

int up_rtc_initialize(void)
{
  int ret = stm32n6_rtc_initialize();
  if (ret >= 0)
    {
      g_rtc_enabled = true;
    }

  return ret;
}

/****************************************************************************
 * Name: up_rtc_getdatetime
 ****************************************************************************/

int up_rtc_getdatetime(struct tm *tp)
{
  return stm32n6_rtc_getdatetime(tp);
}

/****************************************************************************
 * Name: up_rtc_time
 ****************************************************************************/

time_t up_rtc_time(void)
{
  struct tm tm;

  if (stm32n6_rtc_getdatetime(&tm) < 0)
    {
      return 0;
    }

  return timegm(&tm);
}

/****************************************************************************
 * Name: up_rtc_settime
 ****************************************************************************/

int up_rtc_settime(const struct timespec *tp)
{
  struct tm tm;

  if (tp == NULL)
    {
      return -EINVAL;
    }

  gmtime_r(&tp->tv_sec, &tm);
  return stm32n6_rtc_setdatetime(&tm);
}

#endif /* CONFIG_RTC */
