/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_rtc.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_RTC_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_RTC_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* RTC base address (per CMSIS stm32n647xx.h: RTC_BASE_NS) */

#define STM32N6_RTC_BASE        0x46004000ul

/* The battery-backed BKPxR registers physically live in the TAMP block
 * (CMSIS TAMP_TypeDef, BKP0R at TAMP + 0x100), not in the RTC block.
 * TAMP sits directly after RTC on APB4 (RTC + 0x400).
 */

#define STM32N6_TAMP_BASE       0x46004400ul

/* Register Offsets (from Renode STM32N6_RTC.cs) */

#define STM32N6_RTC_TR_OFFSET   0x0000  /* Time register (BCD) */
#define STM32N6_RTC_DR_OFFSET   0x0004  /* Date register (BCD) */
#define STM32N6_RTC_SSR_OFFSET  0x0008  /* Sub-second register (RO) */
#define STM32N6_RTC_ICSR_OFFSET 0x000c  /* Init control/status register */
#define STM32N6_RTC_PRER_OFFSET 0x0010  /* Prescaler register */
#define STM32N6_RTC_WUTR_OFFSET 0x0014  /* Wakeup timer register */
#define STM32N6_RTC_CR_OFFSET   0x0018  /* Control register */
#define STM32N6_RTC_WPR_OFFSET  0x0024  /* Write protection (WO) */
#define STM32N6_RTC_SR_OFFSET   0x0050  /* Status register */

/* Backup registers (BKP0R-BKP19R), offset 0x100 + 4*n from TAMP base */

/* Register Addresses */

#define STM32N6_RTC_TR          (STM32N6_RTC_BASE + STM32N6_RTC_TR_OFFSET)
#define STM32N6_RTC_DR          (STM32N6_RTC_BASE + STM32N6_RTC_DR_OFFSET)
#define STM32N6_RTC_SSR         (STM32N6_RTC_BASE + STM32N6_RTC_SSR_OFFSET)
#define STM32N6_RTC_ICSR        (STM32N6_RTC_BASE + STM32N6_RTC_ICSR_OFFSET)
#define STM32N6_RTC_PRER        (STM32N6_RTC_BASE + STM32N6_RTC_PRER_OFFSET)
#define STM32N6_RTC_WUTR        (STM32N6_RTC_BASE + STM32N6_RTC_WUTR_OFFSET)
#define STM32N6_RTC_CR          (STM32N6_RTC_BASE + STM32N6_RTC_CR_OFFSET)
#define STM32N6_RTC_WPR         (STM32N6_RTC_BASE + STM32N6_RTC_WPR_OFFSET)
#define STM32N6_RTC_SR          (STM32N6_RTC_BASE + STM32N6_RTC_SR_OFFSET)

#define STM32N6_RTC_BKP0R       (STM32N6_TAMP_BASE + 0x100)
#define STM32N6_RTC_BKP1R       (STM32N6_TAMP_BASE + 0x104)
#define STM32N6_RTC_BKP2R       (STM32N6_TAMP_BASE + 0x108)
#define STM32N6_RTC_BKP3R       (STM32N6_TAMP_BASE + 0x10c)
#define STM32N6_RTC_BKP4R       (STM32N6_TAMP_BASE + 0x110)
#define STM32N6_RTC_BKP5R       (STM32N6_TAMP_BASE + 0x114)
#define STM32N6_RTC_BKP6R       (STM32N6_TAMP_BASE + 0x118)
#define STM32N6_RTC_BKP7R       (STM32N6_TAMP_BASE + 0x11c)
#define STM32N6_RTC_BKP8R       (STM32N6_TAMP_BASE + 0x120)
#define STM32N6_RTC_BKP9R       (STM32N6_TAMP_BASE + 0x124)
#define STM32N6_RTC_BKP10R      (STM32N6_TAMP_BASE + 0x128)
#define STM32N6_RTC_BKP11R      (STM32N6_TAMP_BASE + 0x12c)
#define STM32N6_RTC_BKP12R      (STM32N6_TAMP_BASE + 0x130)
#define STM32N6_RTC_BKP13R      (STM32N6_TAMP_BASE + 0x134)
#define STM32N6_RTC_BKP14R      (STM32N6_TAMP_BASE + 0x138)
#define STM32N6_RTC_BKP15R      (STM32N6_TAMP_BASE + 0x13c)
#define STM32N6_RTC_BKP16R      (STM32N6_TAMP_BASE + 0x140)
#define STM32N6_RTC_BKP17R      (STM32N6_TAMP_BASE + 0x144)
#define STM32N6_RTC_BKP18R      (STM32N6_TAMP_BASE + 0x148)
#define STM32N6_RTC_BKP19R      (STM32N6_TAMP_BASE + 0x14c)

#define STM32N6_RTC_BKPCOUNT    20

/* RTC Time Register (TR) bit definitions */

#define RTC_TR_SU_SHIFT         (0)       /* Bits 0-3: Second units in BCD */
#define RTC_TR_SU_MASK          (15 << RTC_TR_SU_SHIFT)
#define RTC_TR_ST_SHIFT         (4)       /* Bits 4-6: Second tens in BCD */
#define RTC_TR_ST_MASK          (7 << RTC_TR_ST_SHIFT)
#define RTC_TR_MNU_SHIFT        (8)       /* Bits 8-11: Minute units in BCD */
#define RTC_TR_MNU_MASK         (15 << RTC_TR_MNU_SHIFT)
#define RTC_TR_MNT_SHIFT        (12)      /* Bits 12-14: Minute tens in BCD */
#define RTC_TR_MNT_MASK         (7 << RTC_TR_MNT_SHIFT)
#define RTC_TR_HU_SHIFT         (16)      /* Bits 16-19: Hour units in BCD */
#define RTC_TR_HU_MASK          (15 << RTC_TR_HU_SHIFT)
#define RTC_TR_HT_SHIFT         (20)      /* Bits 20-21: Hour tens in BCD */
#define RTC_TR_HT_MASK          (3 << RTC_TR_HT_SHIFT)
#define RTC_TR_PM               (1 << 22) /* Bit 22: AM/PM notation */
#define RTC_TR_RESERVED_BITS    (0xff808080)

/* RTC Date Register (DR) bit definitions */

#define RTC_DR_DU_SHIFT         (0)       /* Bits 0-3: Date units in BCD */
#define RTC_DR_DU_MASK          (15 << RTC_DR_DU_SHIFT)
#define RTC_DR_DT_SHIFT         (4)       /* Bits 4-5: Date tens in BCD */
#define RTC_DR_DT_MASK          (3 << RTC_DR_DT_SHIFT)
#define RTC_DR_MU_SHIFT         (8)       /* Bits 8-11: Month units in BCD */
#define RTC_DR_MU_MASK          (15 << RTC_DR_MU_SHIFT)
#define RTC_DR_MT               (1 << 12) /* Bit 12: Month tens in BCD */
#define RTC_DR_WDU_SHIFT        (13)      /* Bits 13-15: Week day units */
#define RTC_DR_WDU_MASK         (7 << RTC_DR_WDU_SHIFT)
#define RTC_DR_YU_SHIFT         (16)      /* Bits 16-19: Year units in BCD */
#define RTC_DR_YU_MASK          (15 << RTC_DR_YU_SHIFT)
#define RTC_DR_YT_SHIFT         (20)      /* Bits 20-23: Year tens in BCD */
#define RTC_DR_YT_MASK          (15 << RTC_DR_YT_SHIFT)
#define RTC_DR_RESERVED_BITS    (0xff0000c0)

/* RTC Init Control/Status Register (ICSR) bit definitions */

#define RTC_ICSR_ALRAWF         (1 << 0)  /* Bit 0: Alarm A write flag */
#define RTC_ICSR_ALRBWF         (1 << 1)  /* Bit 1: Alarm B write flag */
#define RTC_ICSR_WUTWF          (1 << 2)  /* Bit 2: Wakeup timer write flag */
#define RTC_ICSR_SHPF           (1 << 3)  /* Bit 3: Shift operation pending */
#define RTC_ICSR_INITS          (1 << 4)  /* Bit 4: Initialization status */
#define RTC_ICSR_RSF            (1 << 5)  /* Bit 5: Reg sync flag */
#define RTC_ICSR_INITF          (1 << 6)  /* Bit 6: Init flag */
#define RTC_ICSR_INIT           (1 << 7)  /* Bit 7: Init mode */

/* RTC Control Register (CR) bit definitions */

#define RTC_CR_WUCKSEL_SHIFT    (0)       /* Bits 0-2: Wakeup clock sel */
#define RTC_CR_WUCKSEL_MASK     (7 << RTC_CR_WUCKSEL_SHIFT)
#define RTC_CR_TSEDGE           (1 << 3)  /* Bit 3: Timestamp edge */
#define RTC_CR_REFCKON          (1 << 4)  /* Bit 4: Reference clock enable */
#define RTC_CR_BYPSHAD          (1 << 5)  /* Bit 5: Bypass shadow regs */
#define RTC_CR_FMT              (1 << 6)  /* Bit 6: Hour format (0=24h) */
#define RTC_CR_ALRAE            (1 << 8)  /* Bit 8: Alarm A enable */
#define RTC_CR_ALRBE            (1 << 9)  /* Bit 9: Alarm B enable */
#define RTC_CR_WUTE             (1 << 10) /* Bit 10: Wakeup timer enable */
#define RTC_CR_TSE              (1 << 11) /* Bit 11: Timestamp enable */
#define RTC_CR_ALRAIE           (1 << 12) /* Bit 12: Alarm A IRQ enable */
#define RTC_CR_ALRBIE           (1 << 13) /* Bit 13: Alarm B IRQ enable */
#define RTC_CR_WUTIE            (1 << 14) /* Wakeup timer IRQ enable */

/* RTC Prescaler Register (PRER) bit definitions */

#define RTC_PRER_PREDIV_S_SHIFT (0)       /* Bits 0-14: Sync prescaler */
#define RTC_PRER_PREDIV_S_MASK  (0x7fff << RTC_PRER_PREDIV_S_SHIFT)
#define RTC_PRER_PREDIV_A_SHIFT (16)      /* Bits 16-22: Async prescaler */
#define RTC_PRER_PREDIV_A_MASK  (0x7f << RTC_PRER_PREDIV_A_SHIFT)

/* RTC Status Register (SR) bit definitions */

#define RTC_SR_ALRAF            (1 << 0)  /* Bit 0: Alarm A flag */
#define RTC_SR_ALRBF            (1 << 1)  /* Bit 1: Alarm B flag */
#define RTC_SR_WUTF             (1 << 2)  /* Bit 2: Wakeup timer flag */
#define RTC_SR_TSF              (1 << 3)  /* Bit 3: Timestamp flag */
#define RTC_SR_TSOVF            (1 << 4)  /* Bit 4: Timestamp overflow */
#define RTC_SR_ITSF             (1 << 5)  /* Bit 5: Internal timestamp flag */

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_RTC_H */
