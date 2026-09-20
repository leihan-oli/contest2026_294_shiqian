/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_tim.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_TIM_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_TIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Name: stm32n6_timer_initialize
 *
 * Description:
 *   Bind a general-purpose timer to a character device and register it
 *   with the NuttX timer framework at the given device path.
 *
 * Input Parameters:
 *   devpath - Character device path (e.g. "/dev/timer0").
 *   timer   - Timer peripheral number (currently only 2 is supported).
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_timer_initialize(const char *devpath, int timer);

#ifdef CONFIG_STM32_TIM3_PWM

/****************************************************************************
 * Name: stm32n6_tim_pwm_initialize
 *
 * Description:
 *   Register TIM3 as a PWM character device (e.g. "/dev/pwm1"), driving the
 *   TIM3_CH1 compare output.  The waveform is generated internally and can
 *   be routed on-chip into TIM15 TI1 (via TISEL) for a wire-free capture
 *   loopback; no external pin is claimed.
 *
 * Input Parameters:
 *   devpath - Character device path (e.g. "/dev/pwm1").
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_tim_pwm_initialize(const char *devpath);

#endif /* CONFIG_STM32_TIM3_PWM */

#ifdef CONFIG_STM32_TIM15_CAP

/****************************************************************************
 * Name: stm32n6_tim_cap_initialize
 *
 * Description:
 *   Register TIM15 as an input-capture character device (e.g. "/dev/cap0")
 *   measuring frequency and duty cycle.  TIM15 runs in PWM-input mode with
 *   its TI1 sourced internally from TIM3 CH1 (TISEL loopback), so the
 *   TIM3 PWM waveform is read back with zero external wiring.
 *
 * Input Parameters:
 *   devpath - Character device path (e.g. "/dev/cap0").
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_tim_cap_initialize(const char *devpath);

#endif /* CONFIG_STM32_TIM15_CAP */

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_TIM_H */
