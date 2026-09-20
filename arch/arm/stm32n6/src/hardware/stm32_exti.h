/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_exti.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_EXTI_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_EXTI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register offsets (from CMSIS stm32n647xx.h EXTI_TypeDef).  Only the
 * subset used by the LPTIM Stop-mode wakeup path is declared here: the
 * rising/falling pending registers for lines 32..63 and the interrupt
 * mask register 2.
 */

#define STM32_EXTI_RPR2_OFFSET  0x002c  /* Rising pending register 2 */
#define STM32_EXTI_FPR2_OFFSET  0x0030  /* Falling pending register 2 */
#define STM32_EXTI_IMR2_OFFSET  0x0090  /* Interrupt mask register 2 */

/* Register addresses *******************************************************/

#define STM32_EXTI_RPR2   (STM32_EXTI_BASE + STM32_EXTI_RPR2_OFFSET)
#define STM32_EXTI_FPR2   (STM32_EXTI_BASE + STM32_EXTI_FPR2_OFFSET)
#define STM32_EXTI_IMR2   (STM32_EXTI_BASE + STM32_EXTI_IMR2_OFFSET)

/* LPTIM wakeup lines in IMR2/RPR2/FPR2 (line number - 32).  The LPTIMx
 * interrupt reaches the CPU as an EXTI direct line; to wake the CPU from
 * Stop mode the corresponding IMR2 line must be unmasked (unmasking the
 * NVIC IRQ alone is not sufficient).  Lines: LPTIM1=52, LPTIM2=53,
 * LPTIM3=55, LPTIM4=57, LPTIM5=58.
 */

#define EXTI_IMR2_LPTIM1  (1 << 20)  /* Line 52 */
#define EXTI_IMR2_LPTIM2  (1 << 21)  /* Line 53 */
#define EXTI_IMR2_LPTIM3  (1 << 23)  /* Line 55 */
#define EXTI_IMR2_LPTIM4  (1 << 25)  /* Line 57 */
#define EXTI_IMR2_LPTIM5  (1 << 26)  /* Line 58 */

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_EXTI_H */
