/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_lptim.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_LPTIM_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_LPTIM_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register offsets *********************************************************/

#define STM32_LPTIM_ISR_OFFSET    0x0000  /* Interrupt and status register */
#define STM32_LPTIM_ICR_OFFSET    0x0004  /* Interrupt clear register */
#define STM32_LPTIM_DIER_OFFSET   0x0008  /* Interrupt enable register */
#define STM32_LPTIM_CFGR_OFFSET   0x000c  /* Configuration register */
#define STM32_LPTIM_CR_OFFSET     0x0010  /* Control register */
#define STM32_LPTIM_CCR1_OFFSET   0x0014  /* Capture/compare register 1 */
#define STM32_LPTIM_ARR_OFFSET    0x0018  /* Autoreload register */
#define STM32_LPTIM_CNT_OFFSET    0x001c  /* Counter register */
#define STM32_LPTIM_RCR_OFFSET    0x0028  /* Repetition register */
#define STM32_LPTIM_CCMR1_OFFSET  0x002c  /* Capture/compare mode reg 1 */

/* Register addresses *******************************************************/

#define STM32_LPTIM1_ISR   (STM32_LPTIM1_BASE + STM32_LPTIM_ISR_OFFSET)
#define STM32_LPTIM1_ICR   (STM32_LPTIM1_BASE + STM32_LPTIM_ICR_OFFSET)
#define STM32_LPTIM1_DIER  (STM32_LPTIM1_BASE + STM32_LPTIM_DIER_OFFSET)
#define STM32_LPTIM1_CFGR  (STM32_LPTIM1_BASE + STM32_LPTIM_CFGR_OFFSET)
#define STM32_LPTIM1_CR    (STM32_LPTIM1_BASE + STM32_LPTIM_CR_OFFSET)
#define STM32_LPTIM1_ARR   (STM32_LPTIM1_BASE + STM32_LPTIM_ARR_OFFSET)
#define STM32_LPTIM1_CNT   (STM32_LPTIM1_BASE + STM32_LPTIM_CNT_OFFSET)

/* Register bit definitions *************************************************/

/* Interrupt and status register (ISR) */

#define LPTIM_ISR_CC1IF   (1 << 0)   /* Capture/compare 1 interrupt flag */
#define LPTIM_ISR_ARRM    (1 << 1)   /* Autoreload match */
#define LPTIM_ISR_CMP1OK  (1 << 3)   /* Compare 1 register update OK */
#define LPTIM_ISR_ARROK   (1 << 4)   /* Autoreload register update OK */
#define LPTIM_ISR_REPOK   (1 << 5)   /* Repetition register update OK */

/* Interrupt clear register (ICR) */

#define LPTIM_ICR_CC1IFCF  (1 << 0)  /* Capture/compare 1 clear flag */
#define LPTIM_ICR_ARRMCF   (1 << 1)  /* Autoreload match clear flag */
#define LPTIM_ICR_CMP1OKCF (1 << 3)  /* Compare 1 update-OK clear flag */
#define LPTIM_ICR_ARROKCF  (1 << 4)  /* Autoreload update OK clear flag */
#define LPTIM_ICR_REPOKCF  (1 << 5)  /* Repetition update-OK clear flag */

/* Interrupt enable register (DIER) */

#define LPTIM_DIER_CC1IE  (1 << 0)   /* Capture/compare 1 interrupt enable */
#define LPTIM_DIER_ARRMIE (1 << 1)   /* Autoreload match interrupt enable */

/* Configuration register (CFGR) */

#define LPTIM_CFGR_CKSEL    (1 << 0)   /* Clock selector (0 = internal) */
#define LPTIM_CFGR_PRESC_SHIFT 9       /* Clock prescaler PRESC[2:0] */
#define LPTIM_CFGR_PRESC_MASK  (0x7 << LPTIM_CFGR_PRESC_SHIFT)
#define LPTIM_CFGR_WAVE     (1 << 20)  /* Waveform shape (0 = PWM/one-shot) */
#define LPTIM_CFGR_WAVPOL   (1 << 21)  /* Waveform shape polarity */
#define LPTIM_CFGR_PRELOAD  (1 << 22)  /* ARR/CCR update mode (1 = at UE) */

/* Control register (CR) */

#define LPTIM_CR_ENABLE   (1 << 0)   /* LPTIM enable */
#define LPTIM_CR_SNGSTRT  (1 << 1)   /* Start in single mode */
#define LPTIM_CR_CNTSTRT  (1 << 2)   /* Start in continuous mode */

/* Capture/compare mode register 1 (CCMR1) */

#define LPTIM_CCMR1_CC1SEL (1 << 0)  /* Capture/compare 1 selection (0=out) */
#define LPTIM_CCMR1_CC1E   (1 << 1)  /* Capture/compare 1 output enable */
#define LPTIM_CCMR1_CC1P_SHIFT 2     /* Capture/compare 1 output polarity */
#define LPTIM_CCMR1_CC1P_MASK  (0x3 << LPTIM_CCMR1_CC1P_SHIFT)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_LPTIM_H */
