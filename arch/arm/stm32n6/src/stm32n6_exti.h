/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_exti.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_EXTI_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_EXTI_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

#include <nuttx/irq.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* EXTI base address (non-secure, AHB4 bus) */

#define STM32N6_EXTI_BASE             0x46020000ul

/* EXTI register offsets - Bank 1 (lines 0-21) */

#define STM32N6_EXTI_RTSR1_OFFSET     0x0000
#define STM32N6_EXTI_FTSR1_OFFSET     0x0004
#define STM32N6_EXTI_SWIER1_OFFSET    0x0008
#define STM32N6_EXTI_RPR1_OFFSET      0x000c
#define STM32N6_EXTI_FPR1_OFFSET      0x0010
#define STM32N6_EXTI_SECCFGR1_OFFSET  0x0014
#define STM32N6_EXTI_PRIVCFGR1_OFFSET 0x0018

/* EXTI register offsets - Bank 2 (lines 32+) */

#define STM32N6_EXTI_RTSR2_OFFSET     0x0020
#define STM32N6_EXTI_FTSR2_OFFSET     0x0024
#define STM32N6_EXTI_SWIER2_OFFSET    0x0028
#define STM32N6_EXTI_RPR2_OFFSET      0x002c
#define STM32N6_EXTI_FPR2_OFFSET      0x0030
#define STM32N6_EXTI_SECCFGR2_OFFSET  0x0034
#define STM32N6_EXTI_PRIVCFGR2_OFFSET 0x0038

/* EXTI register offsets - Bank 3 (lines 64+) */

#define STM32N6_EXTI_RTSR3_OFFSET     0x0040
#define STM32N6_EXTI_FTSR3_OFFSET     0x0044
#define STM32N6_EXTI_SWIER3_OFFSET    0x0048
#define STM32N6_EXTI_RPR3_OFFSET      0x004c
#define STM32N6_EXTI_FPR3_OFFSET      0x0050
#define STM32N6_EXTI_SECCFGR3_OFFSET  0x0054
#define STM32N6_EXTI_PRIVCFGR3_OFFSET 0x0058

/* EXTICR register offsets */

#define STM32N6_EXTI_EXTICR0_OFFSET   0x0060
#define STM32N6_EXTI_EXTICR1_OFFSET   0x0064
#define STM32N6_EXTI_EXTICR2_OFFSET   0x0068
#define STM32N6_EXTI_EXTICR3_OFFSET   0x006c

/* Lock register offset */

#define STM32N6_EXTI_LOCKR_OFFSET     0x0070

/* Interrupt/event mask register offsets */

#define STM32N6_EXTI_IMR1_OFFSET      0x0080
#define STM32N6_EXTI_EMR1_OFFSET      0x0084
#define STM32N6_EXTI_IMR2_OFFSET      0x0090
#define STM32N6_EXTI_EMR2_OFFSET      0x0094
#define STM32N6_EXTI_IMR3_OFFSET      0x00a0
#define STM32N6_EXTI_EMR3_OFFSET      0x00a4

/* Register addresses - Bank 1 */

#define STM32N6_EXTI_RTSR1 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_RTSR1_OFFSET)
#define STM32N6_EXTI_FTSR1 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_FTSR1_OFFSET)
#define STM32N6_EXTI_SWIER1 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_SWIER1_OFFSET)
#define STM32N6_EXTI_RPR1 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_RPR1_OFFSET)
#define STM32N6_EXTI_FPR1 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_FPR1_OFFSET)

/* Register addresses - Bank 2 */

#define STM32N6_EXTI_RTSR2 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_RTSR2_OFFSET)
#define STM32N6_EXTI_FTSR2 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_FTSR2_OFFSET)
#define STM32N6_EXTI_SWIER2 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_SWIER2_OFFSET)
#define STM32N6_EXTI_RPR2 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_RPR2_OFFSET)
#define STM32N6_EXTI_FPR2 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_FPR2_OFFSET)

/* Register addresses - Bank 3 */

#define STM32N6_EXTI_RTSR3 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_RTSR3_OFFSET)
#define STM32N6_EXTI_FTSR3 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_FTSR3_OFFSET)
#define STM32N6_EXTI_SWIER3 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_SWIER3_OFFSET)
#define STM32N6_EXTI_RPR3 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_RPR3_OFFSET)
#define STM32N6_EXTI_FPR3 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_FPR3_OFFSET)

/* Register addresses - EXTICR and lock */

#define STM32N6_EXTI_EXTICR(n) \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_EXTICR0_OFFSET + ((n) << 2))
#define STM32N6_EXTI_LOCKR \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_LOCKR_OFFSET)

/* Register addresses - Mask registers */

#define STM32N6_EXTI_IMR1 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_IMR1_OFFSET)
#define STM32N6_EXTI_EMR1 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_EMR1_OFFSET)
#define STM32N6_EXTI_IMR2 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_IMR2_OFFSET)
#define STM32N6_EXTI_EMR2 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_EMR2_OFFSET)
#define STM32N6_EXTI_IMR3 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_IMR3_OFFSET)
#define STM32N6_EXTI_EMR3 \
  (STM32N6_EXTI_BASE + STM32N6_EXTI_EMR3_OFFSET)

/* Helper macros for EXTI line operations */

#define STM32N6_EXTI_LINE_MASK(line)    (1u << ((line) & 0x1f))
#define STM32N6_EXTI_EXTICR_INDEX(line) ((line) >> 2)
#define STM32N6_EXTI_EXTICR_SHIFT(line) (((line) & 3) << 3)
#define STM32N6_EXTI_EXTICR_MASK(line) \
  (0xffu << STM32N6_EXTI_EXTICR_SHIFT(line))

/* Number of GPIO EXTI lines (0-15 map to GPIO pins) */

#define STM32N6_EXTI_NUM_GPIO_LINES   16

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

#ifndef __ASSEMBLY__

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/****************************************************************************
 * Name: stm32n6_exti_initialize
 *
 * Description:
 *   Initialize the EXTI controller. Called once during system startup.
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_exti_initialize(void);

/****************************************************************************
 * Name: stm32n6_gpiosetevent
 *
 * Description:
 *   Sets/clears GPIO-based event and interrupt triggers on an EXTI line.
 *
 * Input Parameters:
 *   - pinset:      GPIO pin configuration (encodes port and pin number)
 *   - risingedge:  Enable interrupt on rising edges
 *   - fallingedge: Enable interrupt on falling edges
 *   - event:       Generate event when set
 *   - func:        When non-NULL, interrupt callback handler
 *   - arg:         Argument passed to the interrupt callback
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_gpiosetevent(uint32_t pinset, bool risingedge,
                         bool fallingedge, bool event,
                         xcpt_t func, void *arg);

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_EXTI_H */
