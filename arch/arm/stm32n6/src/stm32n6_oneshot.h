/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_oneshot.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_ONESHOT_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_ONESHOT_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/timers/oneshot.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Name: stm32n6_oneshot_initialize
 *
 * Description:
 *   Initialize a one-shot timer and return its lower-half instance for
 *   registration with the NuttX oneshot framework via oneshot_register().
 *
 * Input Parameters:
 *   timer - Timer peripheral number (currently only 5 is supported).
 *
 * Returned Value:
 *   A pointer to the lower-half instance on success; NULL on failure.
 *
 ****************************************************************************/

struct oneshot_lowerhalf_s *stm32n6_oneshot_initialize(int timer);

#ifdef __cplusplus
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_ONESHOT_H */
