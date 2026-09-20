/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_rtc.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_RTC_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_RTC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/clock.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Default prescaler for LSI ~32kHz clock:
 * PREDIV_A = 127 (async prescaler: /128)
 * PREDIV_S = 249 (sync prescaler: /250)
 * 32000 / (128 * 250) = 1 Hz (1 second tick)
 */

#define STM32N6_RTC_PREDIV_A_DEFAULT  127
#define STM32N6_RTC_PREDIV_S_DEFAULT  249

/* Magic value stored in backup register 0 to detect cold boot */

#define STM32N6_RTC_MAGIC             0xfacefeed
#define STM32N6_RTC_MAGIC_TIME_SET    0xf00dface

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

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

int stm32n6_rtc_getdatetime(struct tm *tp);

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

int stm32n6_rtc_setdatetime(const struct tm *tp);

/****************************************************************************
 * Name: stm32n6_rtc_initialize
 *
 * Description:
 *   Initialize the RTC peripheral.  This function should be called from
 *   board_late_initialize() or similar board init code.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno on failure
 *
 ****************************************************************************/

int stm32n6_rtc_initialize(void);

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

bool stm32n6_rtc_havesettime(void);

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

#ifdef CONFIG_RTC_DRIVER
struct rtc_lowerhalf_s *stm32n6_rtc_lowerhalf(void);
#endif

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_RTC_H */
