/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_tim.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_TIM_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_TIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets (common to all general-purpose timers) ******************/

#define STM32_TIM_CR1_OFFSET     0x0000  /* Control register 1 */
#define STM32_TIM_CR2_OFFSET     0x0004  /* Control register 2 */
#define STM32_TIM_SMCR_OFFSET    0x0008  /* Slave mode control register */
#define STM32_TIM_DIER_OFFSET    0x000c  /* DMA/interrupt enable register */
#define STM32_TIM_SR_OFFSET      0x0010  /* Status register */
#define STM32_TIM_EGR_OFFSET     0x0014  /* Event generation register */
#define STM32_TIM_CCMR1_OFFSET   0x0018  /* Capture/compare mode register 1 */
#define STM32_TIM_CCER_OFFSET    0x0020  /* Capture/compare enable register */
#define STM32_TIM_CNT_OFFSET     0x0024  /* Counter register */
#define STM32_TIM_PSC_OFFSET     0x0028  /* Prescaler register */
#define STM32_TIM_ARR_OFFSET     0x002c  /* Auto-reload register */
#define STM32_TIM_CCR1_OFFSET    0x0034  /* Capture/compare register 1 */
#define STM32_TIM_CCR2_OFFSET    0x0038  /* Capture/compare register 2 */
#define STM32_TIM_TISEL_OFFSET   0x005c  /* Timer input selection register */

/* Register Addresses *******************************************************/

/* TIM2 (32-bit general-purpose timer) on APB1 @ 0x40000000 */

#define STM32_TIM2_CR1           (STM32_TIM2_BASE + STM32_TIM_CR1_OFFSET)
#define STM32_TIM2_CR2           (STM32_TIM2_BASE + STM32_TIM_CR2_OFFSET)
#define STM32_TIM2_SMCR          (STM32_TIM2_BASE + STM32_TIM_SMCR_OFFSET)
#define STM32_TIM2_DIER          (STM32_TIM2_BASE + STM32_TIM_DIER_OFFSET)
#define STM32_TIM2_SR            (STM32_TIM2_BASE + STM32_TIM_SR_OFFSET)
#define STM32_TIM2_EGR           (STM32_TIM2_BASE + STM32_TIM_EGR_OFFSET)
#define STM32_TIM2_CNT           (STM32_TIM2_BASE + STM32_TIM_CNT_OFFSET)
#define STM32_TIM2_PSC           (STM32_TIM2_BASE + STM32_TIM_PSC_OFFSET)
#define STM32_TIM2_ARR           (STM32_TIM2_BASE + STM32_TIM_ARR_OFFSET)

/* Register Bit Definitions *************************************************/

/* Control register 1 (CR1) */

#define TIM_CR1_CEN              (1 << 0)  /* Counter enable */
#define TIM_CR1_UDIS             (1 << 1)  /* Update disable */
#define TIM_CR1_URS              (1 << 2)  /* Update request source */
#define TIM_CR1_OPM              (1 << 3)  /* One-pulse mode */
#define TIM_CR1_ARPE             (1 << 7)  /* Auto-reload preload enable */

/* DMA/interrupt enable register (DIER) */

#define TIM_DIER_UIE             (1 << 0)  /* Update interrupt enable */
#define TIM_DIER_CC1IE           (1 << 1)  /* Capture/compare 1 int enable */

/* Status register (SR) */

#define TIM_SR_UIF               (1 << 0)  /* Update interrupt flag */
#define TIM_SR_CC1IF             (1 << 1)  /* Capture/compare 1 int flag */

/* Event generation register (EGR) */

#define TIM_EGR_UG               (1 << 0)  /* Update generation */

/* Capture/compare enable register (CCER) */

#define TIM_CCER_CC1E            (1 << 0)  /* Capture/compare 1 enable */
#define TIM_CCER_CC1P            (1 << 1)  /* Capture/compare 1 polarity */
#define TIM_CCER_CC1NP           (1 << 3)  /* Capture/compare 1 compl. pol. */
#define TIM_CCER_CC2E            (1 << 4)  /* Capture/compare 2 enable */
#define TIM_CCER_CC2P            (1 << 5)  /* Capture/compare 2 polarity */

/* DMA/interrupt enable register (DIER) — capture/compare 2 */

#define TIM_DIER_CC2IE           (1 << 2)  /* Capture/compare 2 int enable */

/* Status register (SR) — capture/compare 2 */

#define TIM_SR_CC2IF             (1 << 2)  /* Capture/compare 2 int flag */

/* Capture/compare mode register 1 (CCMR1) — output-compare view.
 * OC1M = 0b110 selects PWM mode 1 (active while CNT < CCR1).
 */

#define TIM_CCMR1_OC1PE          (1 << 3)  /* Output-compare 1 preload en */
#define TIM_CCMR1_OC1M_PWM1      (6 << 4)  /* OC1M[2:0] = PWM mode 1 */
#define TIM_CCMR1_OC1M_MASK      (7 << 4)  /* OC1M[2:0] field mask */

/* Capture/compare mode register 1 (CCMR1) — input-capture view.
 * CC1S/CC2S = 0b01/0b10 map the capture channels onto input TI1.
 */

#define TIM_CCMR1_CC1S_TI1       (1 << 0)  /* CC1S[1:0]: CC1 mapped on TI1 */
#define TIM_CCMR1_CC1S_MASK      (3 << 0)
#define TIM_CCMR1_CC2S_TI1       (2 << 8)  /* CC2S[1:0]: CC2 mapped on TI1 */
#define TIM_CCMR1_CC2S_MASK      (3 << 8)

/* Control register 2 (CR2).  MMS selects the trigger output (TRGO):
 * 0b010 = update event (one pulse per counter period, time-base only);
 * 0b100 = OC1REF (the PWM compare waveform itself, one rising edge per
 * period).  OC1REF is used for the loopback so the readback exercises the
 * output-compare engine, not merely the counter overflow.
 */

#define TIM_CR2_MMS_UPDATE       (2 << 4)  /* MMS[2:0] = update -> TRGO */
#define TIM_CR2_MMS_OC1REF       (4 << 4)  /* MMS[2:0] = OC1REF -> TRGO */
#define TIM_CR2_MMS_MASK         (7 << 4)

/* Slave mode control register (SMCR).  Reset mode (SMS = 0b100) reloads
 * the counter on the selected trigger; external clock mode 1 (SMS = 0b111)
 * clocks the counter from the selected trigger.  TS selects the trigger:
 * TI1FP1 (0b00101) for TI capture, ITR2 (0b00010) for the tim3_trgo
 * inter-timer link.
 */

#define TIM_SMCR_SMS_RESET       (4 << 0)  /* SMS[2:0] = reset mode */
#define TIM_SMCR_SMS_EXTCLK1     (7 << 0)  /* SMS[2:0] = ext clock mode 1 */
#define TIM_SMCR_SMS_MASK        (7 << 0)
#define TIM_SMCR_TS_TI1FP1       (5 << 4)  /* TS[2:0] = TI1FP1 (0b101) */
#define TIM_SMCR_TS_ITR2         (2 << 4)  /* TS[2:0] = ITR2 (0b010) */
#define TIM_SMCR_TS_MASK         (7 << 4)

/* Timer input selection register (TISEL).  On TIM15, TI1SEL = 0b0010
 * routes TIM3 CH1 internally into TI1 (no external pin).
 */

#define TIM_TISEL_TI1SEL_MASK    (0xf << 0)
#define TIM_TISEL_TI1_TIM3_CH1   (0x2 << 0)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_TIM_H */
