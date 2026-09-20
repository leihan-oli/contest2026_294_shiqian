/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_wwdg.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_WWDG_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_WWDG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register offsets *********************************************************/

#define STM32_WWDG_CR_OFFSET     0x0000  /* Control register */
#define STM32_WWDG_CFR_OFFSET    0x0004  /* Configuration register */
#define STM32_WWDG_SR_OFFSET     0x0008  /* Status register */

/* Register addresses *******************************************************/

#define STM32_WWDG_CR            (STM32_WWDG_BASE + STM32_WWDG_CR_OFFSET)
#define STM32_WWDG_CFR           (STM32_WWDG_BASE + STM32_WWDG_CFR_OFFSET)
#define STM32_WWDG_SR            (STM32_WWDG_BASE + STM32_WWDG_SR_OFFSET)

/* Register bit definitions *************************************************/

/* Control register (CR) */

#define WWDG_CR_T_SHIFT          0          /* 7-bit down-counter */
#define WWDG_CR_T_MASK           (0x7f << WWDG_CR_T_SHIFT)
#define WWDG_CR_T_RESET          0x7f       /* Counter reload value */
#define WWDG_CR_WDGA             (1 << 7)   /* Activation bit */

/* Configuration register (CFR) */

#define WWDG_CFR_W_SHIFT         0          /* 7-bit window value */
#define WWDG_CFR_W_MASK          (0x7f << WWDG_CFR_W_SHIFT)
#define WWDG_CFR_EWI             (1 << 9)   /* Early-wakeup interrupt */
#define WWDG_CFR_WDGTB_SHIFT     11         /* Timer base prescaler */
#define WWDG_CFR_WDGTB_MASK      (0x7 << WWDG_CFR_WDGTB_SHIFT)

/* Status register (SR) */

#define WWDG_SR_EWIF             (1 << 0)   /* Early-wakeup int flag */

/* Counter bounds: the 7-bit counter resets the MCU when it rolls under
 * 0x3f (the T6 bit clears), so the usable window spans 0x40..0x7f.
 */

#define WWDG_CR_T_MIN            0x40       /* Reset threshold (T6 clear) */
#define WWDG_CR_T_MAX            0x7f       /* Maximum reload */

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_WWDG_H */
