/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_dts.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_DTS_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_DTS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_STM32_DTS

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: stm32n6_dts_initialize
 *
 * Description:
 *   Initialize the digital temperature sensor (DTS) and register a
 *   character device that returns the die junction temperature as a
 *   b16_t (Q16.16) Celsius value on read().
 *
 * Input Parameters:
 *   devpath - Full path to the device to register, e.g. "/dev/temp0".
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_dts_initialize(FAR const char *devpath);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* CONFIG_STM32_DTS */
#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_DTS_H */
