/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_rng.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_RNG_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_RNG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "chip.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets *********************************************************/

#define STM32_RNG_CR_OFFSET       0x0000  /* Control Register */
#define STM32_RNG_SR_OFFSET       0x0004  /* Status Register */
#define STM32_RNG_DR_OFFSET       0x0008  /* Data Register */
#define STM32_RNG_HTCR_OFFSET     0x0010  /* Health Test Config Register */

/* Register Addresses *******************************************************/

#define STM32_RNG_CR              (STM32_RNG_BASE + STM32_RNG_CR_OFFSET)
#define STM32_RNG_SR              (STM32_RNG_BASE + STM32_RNG_SR_OFFSET)
#define STM32_RNG_DR              (STM32_RNG_BASE + STM32_RNG_DR_OFFSET)
#define STM32_RNG_HTCR            (STM32_RNG_BASE + STM32_RNG_HTCR_OFFSET)

/* RNG_CR Register Bitfield Definitions *************************************/

#define RNG_CR_RNGEN              (1 << 2)  /* Bit 2: RNG enable */
#define RNG_CR_IE                 (1 << 3)  /* Bit 3: Interrupt enable */
#define RNG_CR_CED                (1 << 5)  /* Bit 5: Clock error detection */
#define RNG_CR_CONDRST            (1 << 30) /* Bit 30: Conditioning reset */
#define RNG_CR_CONFIGLOCK         (1 << 31) /* Bit 31: Config lock */

/* RNG_SR Register Bitfield Definitions *************************************/

#define RNG_SR_DRDY               (1 << 0)  /* Bit 0: Data ready */
#define RNG_SR_CECS               (1 << 1)  /* Bit 1: Clock error status */
#define RNG_SR_SECS               (1 << 2)  /* Bit 2: Seed error status */
#define RNG_SR_CEIS               (1 << 5)  /* Bit 5: Clock error int */
#define RNG_SR_SEIS               (1 << 6)  /* Bit 6: Seed error int */

/* Timeout for CONDRST completion (ms) */

#define RNG_CONDRST_TIMEOUT       100

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_RNG_H */
