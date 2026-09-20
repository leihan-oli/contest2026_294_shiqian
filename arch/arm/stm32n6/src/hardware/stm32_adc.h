/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_adc.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_ADC_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_ADC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register offsets *********************************************************/

/* Per-ADC-instance registers (base = ADC1 / ADC2) */

#define STM32_ADC_ISR_OFFSET      0x0000  /* Interrupt and status register */
#define STM32_ADC_IER_OFFSET      0x0004  /* Interrupt enable register */
#define STM32_ADC_CR_OFFSET       0x0008  /* Control register */
#define STM32_ADC_CFGR1_OFFSET    0x000c  /* Configuration register 1 */
#define STM32_ADC_CFGR2_OFFSET    0x0010  /* Configuration register 2 */
#define STM32_ADC_SMPR1_OFFSET    0x0014  /* Sample time register 1 */
#define STM32_ADC_SMPR2_OFFSET    0x0018  /* Sample time register 2 */
#define STM32_ADC_PCSEL_OFFSET    0x001c  /* Channel preselection register */
#define STM32_ADC_SQR1_OFFSET     0x0030  /* Regular sequence register 1 */
#define STM32_ADC_SQR2_OFFSET     0x0034  /* Regular sequence register 2 */
#define STM32_ADC_SQR3_OFFSET     0x0038  /* Regular sequence register 3 */
#define STM32_ADC_SQR4_OFFSET     0x003c  /* Regular sequence register 4 */
#define STM32_ADC_DR_OFFSET       0x0040  /* Regular data register */
#define STM32_ADC_AWD1LTR_OFFSET  0x00a8  /* AWD1 low threshold */
#define STM32_ADC_AWD1HTR_OFFSET  0x00ac  /* AWD1 high threshold */
#define STM32_ADC_OR_OFFSET       0x00d0  /* Option register */

/* Common (shared ADC1/ADC2) registers, base = ADC12 common block */

#define STM32_ADC_CSR_OFFSET      0x0000  /* Common status register */
#define STM32_ADC_CCR_OFFSET      0x0008  /* Common control register */
#define STM32_ADC_CDR_OFFSET      0x000c  /* Common regular data register */

/* Register addresses *******************************************************/

#define STM32_ADC1_ISR    (STM32_ADC1_BASE + STM32_ADC_ISR_OFFSET)
#define STM32_ADC1_IER    (STM32_ADC1_BASE + STM32_ADC_IER_OFFSET)
#define STM32_ADC1_CR     (STM32_ADC1_BASE + STM32_ADC_CR_OFFSET)
#define STM32_ADC1_CFGR1  (STM32_ADC1_BASE + STM32_ADC_CFGR1_OFFSET)
#define STM32_ADC1_CFGR2  (STM32_ADC1_BASE + STM32_ADC_CFGR2_OFFSET)
#define STM32_ADC1_SMPR1  (STM32_ADC1_BASE + STM32_ADC_SMPR1_OFFSET)
#define STM32_ADC1_SMPR2  (STM32_ADC1_BASE + STM32_ADC_SMPR2_OFFSET)
#define STM32_ADC1_PCSEL  (STM32_ADC1_BASE + STM32_ADC_PCSEL_OFFSET)
#define STM32_ADC1_SQR1   (STM32_ADC1_BASE + STM32_ADC_SQR1_OFFSET)
#define STM32_ADC1_DR     (STM32_ADC1_BASE + STM32_ADC_DR_OFFSET)

#define STM32_ADC2_ISR    (STM32_ADC2_BASE + STM32_ADC_ISR_OFFSET)
#define STM32_ADC2_IER    (STM32_ADC2_BASE + STM32_ADC_IER_OFFSET)
#define STM32_ADC2_CR     (STM32_ADC2_BASE + STM32_ADC_CR_OFFSET)
#define STM32_ADC2_CFGR1  (STM32_ADC2_BASE + STM32_ADC_CFGR1_OFFSET)
#define STM32_ADC2_SMPR1  (STM32_ADC2_BASE + STM32_ADC_SMPR1_OFFSET)
#define STM32_ADC2_SMPR2  (STM32_ADC2_BASE + STM32_ADC_SMPR2_OFFSET)
#define STM32_ADC2_PCSEL  (STM32_ADC2_BASE + STM32_ADC_PCSEL_OFFSET)
#define STM32_ADC2_SQR1   (STM32_ADC2_BASE + STM32_ADC_SQR1_OFFSET)
#define STM32_ADC2_DR     (STM32_ADC2_BASE + STM32_ADC_DR_OFFSET)
#define STM32_ADC2_OR     (STM32_ADC2_BASE + STM32_ADC_OR_OFFSET)

#define STM32_ADC12_CCR   (STM32_ADC12_COMMON_BASE + STM32_ADC_CCR_OFFSET)

/* Register bit definitions *************************************************/

/* Interrupt and status register (ISR) */

#define ADC_ISR_ADRDY     (1 << 0)   /* ADC ready */
#define ADC_ISR_EOC       (1 << 2)   /* End of regular conversion */
#define ADC_ISR_EOS       (1 << 3)   /* End of regular sequence */
#define ADC_ISR_OVR       (1 << 4)   /* Overrun */
#define ADC_ISR_AWD1      (1 << 7)   /* Analog watchdog 1 flag */

/* Interrupt enable register (IER) */

#define ADC_IER_EOCIE     (1 << 2)   /* End-of-conversion interrupt enable */
#define ADC_IER_AWD1IE    (1 << 7)   /* Analog watchdog 1 interrupt enable */

/* Control register (CR) */

#define ADC_CR_ADEN       (1 << 0)   /* ADC enable */
#define ADC_CR_ADDIS      (1 << 1)   /* ADC disable */
#define ADC_CR_ADSTART    (1 << 2)   /* Start regular conversion */
#define ADC_CR_ADSTP      (1 << 4)   /* Stop regular conversion */
#define ADC_CR_DEEPPWD    (1 << 29)  /* Deep power-down enable */
#define ADC_CR_ADCALDIF   (1 << 30)  /* Differential-mode calibration */
#define ADC_CR_ADCAL      (1 << 31)  /* ADC calibration */

/* Configuration register 1 (CFGR1) */

/* DMNGT[1:0]: 00 = DR only, 01 = DMA one-shot, 11 = DMA circular */

#define ADC_CFGR1_DMNGT_SHIFT  0
#define ADC_CFGR1_DMNGT_MASK   (0x3 << ADC_CFGR1_DMNGT_SHIFT)
#define ADC_CFGR1_DMNGT_DR     (0x0 << ADC_CFGR1_DMNGT_SHIFT)
#define ADC_CFGR1_DMNGT_DMA1S  (0x1 << ADC_CFGR1_DMNGT_SHIFT)
#define ADC_CFGR1_DMNGT_DMACIR (0x3 << ADC_CFGR1_DMNGT_SHIFT)

#define ADC_CFGR1_CONT         (1 << 13)  /* Continuous conversion mode */
#define ADC_CFGR1_AWD1SGL      (1 << 22)  /* AWD1 single-channel scope */
#define ADC_CFGR1_AWD1EN       (1 << 23)  /* AWD1 enable on regular group */

/* AWD1CH[4:0]: analog-watchdog-1 monitored channel */

#define ADC_CFGR1_AWD1CH_SHIFT 26
#define ADC_CFGR1_AWD1CH_MASK  (0x1f << ADC_CFGR1_AWD1CH_SHIFT)

/* Regular sequence register 1 (SQR1) */

#define ADC_SQR1_L_SHIFT    0        /* Regular sequence length (L[3:0]) */
#define ADC_SQR1_SQ1_SHIFT  6        /* 1st conversion channel (SQ1[4:0]) */
#define ADC_SQR1_SQ2_SHIFT  12       /* 2nd conversion channel (SQ2[4:0]) */

/* Sample time register 2 (SMPR2): channels 10..19, 3 bits each.
 * SMP16 (VBAT) at bit 18, SMP17 (VREFINT / VDDCORE) at bit 21.
 */

#define ADC_SMPR2_SMP16_SHIFT  18
#define ADC_SMPR2_SMP16_MASK   (0x7 << ADC_SMPR2_SMP16_SHIFT)
#define ADC_SMPR2_SMP17_SHIFT  21
#define ADC_SMPR2_SMP17_MASK   (0x7 << ADC_SMPR2_SMP17_SHIFT)
#define ADC_SMP_MAX            0x7   /* Longest sampling time (slow source) */

/* Channel preselection register (PCSEL): one enable bit per channel.  A
 * channel must be preselected here before it can be converted, otherwise
 * its input is not connected to the ADC mux and the data register reads 0.
 */

#define ADC_PCSEL_CH(n)   (1 << (n))

/* Regular data register (DR) */

#define ADC_DR_RDATA_MASK  0xffffffff

/* Common control register (CCR) internal-channel path enables */

#define ADC_CCR_VREFEN    (1 << 22)  /* VrefInt internal path enable */
#define ADC_CCR_VBATEN    (1 << 24)  /* VBAT internal path enable */

/* Option register (OR): VDDCORE is routed to the ADC2 mux by OP2, not by a
 * CCR path-enable bit (unlike VREFINT/VBAT).
 */

#define ADC_OR_OP2        (1 << 2)   /* Internal path to VDDCORE */

/* Internal channel numbers.  VREFINT (ch17) is ADC1-only; VBAT (ch16) and
 * VDDCORE (ch17) are ADC2-only -- VBAT and VDDCORE share the ch17 mux slot
 * across instances but differ by peripheral, so /dev/adc1 (ADC2) can scan
 * VBAT + VDDCORE without any external wiring.
 */

#define ADC_CHANNEL_VREFINT  17
#define ADC_CHANNEL_VBAT     16
#define ADC_CHANNEL_VDDCORE  17

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_ADC_H */
