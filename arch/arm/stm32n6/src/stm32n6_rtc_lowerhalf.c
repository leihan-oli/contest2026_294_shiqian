/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_rtc_lowerhalf.c
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

#include <sys/types.h>
#include <stdbool.h>
#include <errno.h>

#include <nuttx/mutex.h>
#include <nuttx/timers/rtc.h>

#include "stm32n6_rtc.h"

#ifdef CONFIG_RTC_DRIVER

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* This is the private type for the RTC state.  It must be cast compatible
 * with struct rtc_lowerhalf_s.
 */

struct stm32n6_lowerhalf_s
{
  /* Reference to the read-only, lower-half operations vtable */

  const struct rtc_ops_s *ops;

  /* Exclusive access to the RTC device */

  mutex_t devlock;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int  stm32n6_rdtime(struct rtc_lowerhalf_s *lower,
                           struct rtc_time *rtctime);
static int  stm32n6_settime(struct rtc_lowerhalf_s *lower,
                            const struct rtc_time *rtctime);
static bool stm32n6_havesettime(struct rtc_lowerhalf_s *lower);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* STM32N6 RTC driver operations.  Only the mandatory date/time methods are
 * wired; alarm/periodic are left unimplemented until ADR-016 adds them.
 */

static const struct rtc_ops_s g_rtc_ops =
{
  .rdtime      = stm32n6_rdtime,
  .settime     = stm32n6_settime,
  .havesettime = stm32n6_havesettime,
};

/* STM32N6 RTC device state */

static struct stm32n6_lowerhalf_s g_rtc_lowerhalf =
{
  .ops     = &g_rtc_ops,
  .devlock = NXMUTEX_INITIALIZER,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_rdtime
 *
 * Description:
 *   Implements the rdtime() method of the RTC driver interface.
 *
 ****************************************************************************/

static int stm32n6_rdtime(struct rtc_lowerhalf_s *lower,
                          struct rtc_time *rtctime)
{
  struct stm32n6_lowerhalf_s *priv;
  int ret;

  priv = (struct stm32n6_lowerhalf_s *)lower;

  ret = nxmutex_lock(&priv->devlock);
  if (ret < 0)
    {
      return ret;
    }

  /* struct rtc_time is cast compatible with struct tm */

  ret = stm32n6_rtc_getdatetime((struct tm *)rtctime);

  nxmutex_unlock(&priv->devlock);
  return ret;
}

/****************************************************************************
 * Name: stm32n6_settime
 *
 * Description:
 *   Implements the settime() method of the RTC driver interface.
 *
 ****************************************************************************/

static int stm32n6_settime(struct rtc_lowerhalf_s *lower,
                           const struct rtc_time *rtctime)
{
  struct stm32n6_lowerhalf_s *priv;
  int ret;

  priv = (struct stm32n6_lowerhalf_s *)lower;

  ret = nxmutex_lock(&priv->devlock);
  if (ret < 0)
    {
      return ret;
    }

  /* struct rtc_time is cast compatible with struct tm */

  ret = stm32n6_rtc_setdatetime((const struct tm *)rtctime);

  nxmutex_unlock(&priv->devlock);
  return ret;
}

/****************************************************************************
 * Name: stm32n6_havesettime
 *
 * Description:
 *   Implements the havesettime() method of the RTC driver interface.
 *
 ****************************************************************************/

static bool stm32n6_havesettime(struct rtc_lowerhalf_s *lower)
{
  return stm32n6_rtc_havesettime();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_rtc_lowerhalf
 *
 * Description:
 *   Instantiate the RTC lower half driver for the STM32N6.  General usage:
 *
 *     lower = stm32n6_rtc_lowerhalf();
 *     rtc_initialize(0, lower);
 *
 * Returned Value:
 *   On success, a non-NULL RTC lower half handle is returned.  NULL is
 *   returned on any failure.
 *
 ****************************************************************************/

struct rtc_lowerhalf_s *stm32n6_rtc_lowerhalf(void)
{
  return (struct rtc_lowerhalf_s *)&g_rtc_lowerhalf;
}

#endif /* CONFIG_RTC_DRIVER */
