/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_iwdg.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_IWDG_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_IWDG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register offsets *********************************************************/

#define STM32_IWDG_KR_OFFSET     0x0000  /* Key register */
#define STM32_IWDG_PR_OFFSET     0x0004  /* Prescaler register */
#define STM32_IWDG_RLR_OFFSET    0x0008  /* Reload register */
#define STM32_IWDG_SR_OFFSET     0x000c  /* Status register */
#define STM32_IWDG_WINR_OFFSET   0x0010  /* Window register */
#define STM32_IWDG_EWCR_OFFSET   0x0014  /* Early-wakeup interrupt cfg */

/* Register addresses *******************************************************/

#define STM32_IWDG_KR            (STM32_IWDG_BASE + STM32_IWDG_KR_OFFSET)
#define STM32_IWDG_PR            (STM32_IWDG_BASE + STM32_IWDG_PR_OFFSET)
#define STM32_IWDG_RLR           (STM32_IWDG_BASE + STM32_IWDG_RLR_OFFSET)
#define STM32_IWDG_SR            (STM32_IWDG_BASE + STM32_IWDG_SR_OFFSET)
#define STM32_IWDG_WINR          (STM32_IWDG_BASE + STM32_IWDG_WINR_OFFSET)
#define STM32_IWDG_EWCR          (STM32_IWDG_BASE + STM32_IWDG_EWCR_OFFSET)

/* Register bit definitions *************************************************/

/* Key register (KR) */

#define IWDG_KR_KEY_RELOAD       0xaaaa  /* Reload counter (feed) */
#define IWDG_KR_KEY_START        0xcccc  /* Start the watchdog */
#define IWDG_KR_KEY_ACCESS       0x5555  /* Enable PR/RLR/WINR write */

/* Prescaler register (PR) -- divider = 4 << PR, so 0..6 = /4../256 */

#define IWDG_PR_MAX              6       /* Largest supported prescaler */

/* Reload register (RLR): 12-bit down-counter reload value */

#define IWDG_RLR_MAX             0x0fff

/* Status register (SR) */

#define IWDG_SR_PVU              (1 << 0)   /* Prescaler value update */
#define IWDG_SR_RVU              (1 << 1)   /* Reload value update */
#define IWDG_SR_WVU              (1 << 2)   /* Window value update */
#define IWDG_SR_EWU              (1 << 3)   /* Early-wakeup value update */
#define IWDG_SR_ONF              (1 << 8)   /* Watchdog enable status */
#define IWDG_SR_EWIF             (1 << 15)  /* Early-wakeup int flag */

/* Early-wakeup interrupt configuration register (EWCR) */

#define IWDG_EWCR_EWIT_MASK      0x0fff     /* Early-wakeup compare value */
#define IWDG_EWCR_EWIE           (1 << 15)  /* Early-wakeup int enable */

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_IWDG_H */
