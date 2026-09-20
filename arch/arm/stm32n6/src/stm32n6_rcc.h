/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_rcc.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_RCC_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_RCC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_clockconfig
 *
 * Description:
 *   Configure the system clock tree. Enables HSI, optionally HSE and PLL1.
 *
 ****************************************************************************/

void stm32n6_clockconfig(void);

/****************************************************************************
 * Name: stm32n6_get_sysclk
 *
 * Description:
 *   Return the current SYSCLK frequency in Hz.
 *
 ****************************************************************************/

uint32_t stm32n6_get_sysclk(void);

/****************************************************************************
 * Name: stm32n6_get_hclk
 *
 * Description:
 *   Return the AHB bus (HCLK) frequency in Hz.
 *
 ****************************************************************************/

uint32_t stm32n6_get_hclk(void);

/****************************************************************************
 * Name: stm32n6_get_pclk1
 *
 * Description:
 *   Return the APB1 bus frequency in Hz.
 *
 ****************************************************************************/

uint32_t stm32n6_get_pclk1(void);

/****************************************************************************
 * Name: stm32n6_get_pclk2
 *
 * Description:
 *   Return the APB2 bus frequency in Hz.
 *
 ****************************************************************************/

uint32_t stm32n6_get_pclk2(void);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_RCC_H */
