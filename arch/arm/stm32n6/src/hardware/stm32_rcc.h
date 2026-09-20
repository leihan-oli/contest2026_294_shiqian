/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_rcc.h
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
 * Register offsets and bitfields below are cross-checked against
 * CMSIS stm32n647xx.h/stm32n657xx.h RCC_TypeDef (identical on both
 * parts) and against the upstream Apache NuttX STM32N6 port
 * (arch/arm/src/stm32n6/hardware/stm32n6xxx_rcc.h,
 * arch/arm/src/stm32n6/stm32n6xx_rcc.c), which targets STM32N657 --
 * a part sharing the same RCC IP as STM32N647.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_RCC_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_RCC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Register Offsets *********************************************************/

#define STM32_RCC_CR_OFFSET         0x0000  /* Clock control */
#define STM32_RCC_SR_OFFSET         0x0004  /* Clock status */
#define STM32_RCC_CFGR1_OFFSET      0x0020  /* Clock configuration 1 */
#define STM32_RCC_CFGR2_OFFSET      0x0024  /* Clock configuration 2 */
#define STM32_RCC_CCIPR4_OFFSET     0x0150  /* Kernel clk sel 4 (I2C4) */
#define STM32_RCC_CCIPR7_OFFSET     0x015c  /* Kernel clock select 7 (RTC) */
#define STM32_RCC_CCIPR12_OFFSET    0x0170  /* Kernel clk sel 12 (LPTIM1) */
#define STM32_RCC_CCIPR13_OFFSET    0x0174  /* Kernel clock select 13 */

/* PLL1 configuration.  Unlike the legacy STM32Fx/Hx PLL layout, DIVN
 * (feedback divider) is packed into PLL1CFGR1 alongside SEL/DIVM; there
 * is no separate "PLL1CFGR2" multiplier register in the clock
 * configuration sequence used by ST/upstream NuttX, even though CMSIS
 * still exposes a PLL1CFGR2 address (reserved / unused by this driver).
 */

#define STM32_RCC_PLL1CFGR1_OFFSET  0x0080  /* PLL1 SEL/DIVM/DIVN */
#define STM32_RCC_PLL1CFGR3_OFFSET  0x0088  /* PLL1 post-dividers */
#define STM32_RCC_PLL2CFGR1_OFFSET  0x0090  /* PLL2 SEL/DIVM/DIVN */
#define STM32_RCC_PLL2CFGR3_OFFSET  0x0098  /* PLL2 post-dividers */

/* IC (Interconnect) divider configuration registers.  Each ICxCFGR
 * selects a PLLn source and an 8-bit integer divider.  Only the ICs
 * used by the default clock tree (CPU=IC1, SYSCLK=IC2/IC6/IC11) are
 * named here; add more as needed following the same +0x04 stride from
 * IC1CFGR.
 */

#define STM32_RCC_IC1CFGR_OFFSET    0x00c4  /* IC1 config (feeds CPUCLK) */
#define STM32_RCC_IC2CFGR_OFFSET    0x00c8  /* IC2 config (feeds SYSCLK) */
#define STM32_RCC_IC3CFGR_OFFSET    0x00cc  /* IC3 config (XSPI2 kernel) */
#define STM32_RCC_IC6CFGR_OFFSET    0x00d8  /* IC6 config (feeds SYSCLK) */
#define STM32_RCC_IC11CFGR_OFFSET   0x00ec  /* IC11 config (feeds SYSCLK) */
#define STM32_RCC_IC16CFGR_OFFSET   0x0100  /* IC16 config (LTDC kernel) */
#define STM32_RCC_IC17CFGR_OFFSET   0x0104  /* IC17 config (DCMIPP kernel) */
#define STM32_RCC_IC18CFGR_OFFSET   0x0108  /* IC18 config (CSI-2 kernel) */

/* IC divider enable register (write-only via ENSR/ENCR aliases below) */

#define STM32_RCC_DIVENR_OFFSET     0x0240  /* IC divider enable register */

/* Peripheral clock enable registers and their atomic Set/Clear alias
 * pairs (Reference: RM0486 14.5).  A write to xxxENSR performs an
 * atomic OR on the paired xxxENR; a write to xxxENCR performs an
 * atomic AND-NOT.  Prefer the Set/Clear aliases over read-modify-write
 * on the plain ENR address to avoid losing concurrently-set bits.
 */

#define STM32_RCC_MEMENR_OFFSET     0x024c  /* AXI/AHB SRAM clock enable */
#define STM32_RCC_MEMRSTR_OFFSET    0x020c  /* SRAM reset register */
#define STM32_RCC_BUSENR_OFFSET     0x0244  /* embedded bus clock enable */
#define STM32_RCC_AHB1ENR_OFFSET    0x0250  /* AHB1 periph clock enable */
#define STM32_RCC_AHB2ENR_OFFSET    0x0254  /* AHB2 periph clock enable */
#define STM32_RCC_AHB3ENR_OFFSET    0x0258  /* AHB3 periph clock enable */
#define STM32_RCC_AHB4ENR_OFFSET    0x025c  /* AHB4 periph clock enable */
#define STM32_RCC_AHB5ENR_OFFSET    0x0260  /* AHB5 periph clock enable */
#define STM32_RCC_AHB5RSTR_OFFSET   0x0220  /* AHB5 periph reset */
#define STM32_RCC_APB1ENR1_OFFSET   0x0264  /* APB1 periph clock enable 1 */
#define STM32_RCC_APB1ENR2_OFFSET   0x0268  /* APB1 periph clock enable 2 */
#define STM32_RCC_APB1LPENR1_OFFSET 0x02a4  /* APB1 sleep clock enable 1 */
#define STM32_RCC_APB2ENR_OFFSET    0x026c  /* APB2 periph clock enable */
#define STM32_RCC_APB2LPENR_OFFSET  0x02ac  /* APB2 sleep clock enable */
#define STM32_RCC_APB4ENR1_OFFSET   0x0274  /* APB4 periph clock enable 1 */
#define STM32_RCC_APB4LPENR1_OFFSET 0x02b4  /* APB4 sleep clock enable 1 */
#define STM32_RCC_APB4ENR2_OFFSET   0x0278  /* APB4 periph clock enable 2 */
#define STM32_RCC_APB5ENR_OFFSET    0x027c  /* APB5 periph clock enable */
#define STM32_RCC_APB5LPENR_OFFSET  0x02bc  /* APB5 periph LP (sleep) clock enable */

#define STM32_RCC_APB5ENSR_OFFSET   0x0a7c  /* APB5 clock enable set */
#define STM32_RCC_APB5LPENSR_OFFSET 0x0abc  /* APB5 LP (sleep) clock enable set */
#define STM32_RCC_APB5RSTR_OFFSET   0x023c  /* APB5 reset register */
#define STM32_RCC_CCIPR1_OFFSET     0x0144  /* periph clock config 1 (DCMIPP) */

/* Set/Clear register aliases.  DIVENSR/DIVENCR and the ENSR/ENCR
 * pairs live in a separate address region (+0x0800/+0x1000 from the
 * base ENR block) per RM0486; offsets below match the upstream NuttX
 * STM32N6 port.
 */

#define STM32_RCC_DIVENSR_OFFSET    0x0a40  /* IC divider enable set */
#define STM32_RCC_BUSENSR_OFFSET    0x0a44  /* embedded bus clock enable set */
#define STM32_RCC_MEMRSTSR_OFFSET   0x0a0c  /* SRAM reset set */
#define STM32_RCC_MEMRSTCR_OFFSET   0x120c  /* SRAM reset clear */
#define STM32_RCC_AHB2ENSR_OFFSET   0x0a54  /* AHB2 clock enable set */
#define STM32_RCC_AHB1ENSR_OFFSET   0x0a50  /* AHB1 clock enable set */
#define STM32_RCC_AHB3ENSR_OFFSET   0x0a58  /* AHB3 clock enable set */
#define STM32_RCC_MEMENSR_OFFSET    0x0a4c  /* SRAM clock enable set */
#define STM32_RCC_AHB4ENSR_OFFSET   0x0a5c  /* AHB4 clock enable set */
#define STM32_RCC_APB1ENSR1_OFFSET  0x0a64  /* APB1 clock enable set 1 */
#define STM32_RCC_APB2ENSR_OFFSET   0x0a6c  /* APB2 clock enable set */
#define STM32_RCC_APB4ENSR1_OFFSET  0x0a74  /* APB4 clock enable set 1 */
#define STM32_RCC_APB4ENSR2_OFFSET  0x0a78  /* APB4 clock enable set 2 */
#define STM32_RCC_BUSLPENSR_OFFSET  0x0a84  /* Bus LP clock enable set */
#define STM32_RCC_MEMLPENSR_OFFSET  0x0a8c  /* SRAM LP clock enable set */
#define STM32_RCC_APB2LPENSR_OFFSET 0x0aac  /* APB2 LP clock enable set */

#define STM32_RCC_CCR_OFFSET        0x1000  /* Clock control clear */
#define STM32_RCC_APB2ENCR_OFFSET   0x126c  /* APB2 clock enable clear */
#define STM32_RCC_CSR_OFFSET        0x0800  /* Clock control set/status */
#define STM32_RCC_HWRSR_OFFSET      0x0030  /* HW reset status register */
#define STM32_RCC_RSR_OFFSET        0x0034  /* Reset (flag-clear) register */

/* Register Addresses *******************************************************/

#define STM32_RCC_CR         (STM32_RCC_BASE + STM32_RCC_CR_OFFSET)
#define STM32_RCC_SR         (STM32_RCC_BASE + STM32_RCC_SR_OFFSET)
#define STM32_RCC_CFGR1      (STM32_RCC_BASE + STM32_RCC_CFGR1_OFFSET)
#define STM32_RCC_CFGR2      (STM32_RCC_BASE + STM32_RCC_CFGR2_OFFSET)
#define STM32_RCC_CCIPR4     (STM32_RCC_BASE + STM32_RCC_CCIPR4_OFFSET)
#define STM32_RCC_CCIPR7     (STM32_RCC_BASE + STM32_RCC_CCIPR7_OFFSET)
#define STM32_RCC_CCIPR12    (STM32_RCC_BASE + STM32_RCC_CCIPR12_OFFSET)
#define STM32_RCC_CCIPR13    (STM32_RCC_BASE + STM32_RCC_CCIPR13_OFFSET)

#define STM32_RCC_PLL1CFGR1  (STM32_RCC_BASE + STM32_RCC_PLL1CFGR1_OFFSET)
#define STM32_RCC_PLL1CFGR3  (STM32_RCC_BASE + STM32_RCC_PLL1CFGR3_OFFSET)
#define STM32_RCC_PLL2CFGR1  (STM32_RCC_BASE + STM32_RCC_PLL2CFGR1_OFFSET)
#define STM32_RCC_PLL2CFGR3  (STM32_RCC_BASE + STM32_RCC_PLL2CFGR3_OFFSET)

#define STM32_RCC_IC1CFGR    (STM32_RCC_BASE + STM32_RCC_IC1CFGR_OFFSET)
#define STM32_RCC_IC2CFGR    (STM32_RCC_BASE + STM32_RCC_IC2CFGR_OFFSET)
#define STM32_RCC_IC3CFGR    (STM32_RCC_BASE + STM32_RCC_IC3CFGR_OFFSET)
#define STM32_RCC_IC6CFGR    (STM32_RCC_BASE + STM32_RCC_IC6CFGR_OFFSET)
#define STM32_RCC_IC11CFGR   (STM32_RCC_BASE + STM32_RCC_IC11CFGR_OFFSET)
#define STM32_RCC_IC16CFGR   (STM32_RCC_BASE + STM32_RCC_IC16CFGR_OFFSET)
#define STM32_RCC_IC17CFGR   (STM32_RCC_BASE + STM32_RCC_IC17CFGR_OFFSET)
#define STM32_RCC_IC18CFGR   (STM32_RCC_BASE + STM32_RCC_IC18CFGR_OFFSET)

#define STM32_RCC_DIVENR     (STM32_RCC_BASE + STM32_RCC_DIVENR_OFFSET)
#define STM32_RCC_DIVENSR    (STM32_RCC_BASE + STM32_RCC_DIVENSR_OFFSET)
#define STM32_RCC_AHB3ENSR   (STM32_RCC_BASE + STM32_RCC_AHB3ENSR_OFFSET)

#define STM32_RCC_MEMENR     (STM32_RCC_BASE + STM32_RCC_MEMENR_OFFSET)
#define STM32_RCC_MEMRSTR    (STM32_RCC_BASE + STM32_RCC_MEMRSTR_OFFSET)
#define STM32_RCC_MEMRSTSR   (STM32_RCC_BASE + STM32_RCC_MEMRSTSR_OFFSET)
#define STM32_RCC_MEMRSTCR   (STM32_RCC_BASE + STM32_RCC_MEMRSTCR_OFFSET)
#define STM32_RCC_BUSENR     (STM32_RCC_BASE + STM32_RCC_BUSENR_OFFSET)
#define STM32_RCC_BUSENSR    (STM32_RCC_BASE + STM32_RCC_BUSENSR_OFFSET)
#define STM32_RCC_AHB1ENR    (STM32_RCC_BASE + STM32_RCC_AHB1ENR_OFFSET)
#define STM32_RCC_AHB1ENSR   (STM32_RCC_BASE + STM32_RCC_AHB1ENSR_OFFSET)
#define STM32_RCC_AHB2ENR    (STM32_RCC_BASE + STM32_RCC_AHB2ENR_OFFSET)
#define STM32_RCC_AHB3ENR    (STM32_RCC_BASE + STM32_RCC_AHB3ENR_OFFSET)
#define STM32_RCC_AHB4ENR    (STM32_RCC_BASE + STM32_RCC_AHB4ENR_OFFSET)
#define STM32_RCC_AHB5ENR    (STM32_RCC_BASE + STM32_RCC_AHB5ENR_OFFSET)
#define STM32_RCC_AHB5RSTR   (STM32_RCC_BASE + STM32_RCC_AHB5RSTR_OFFSET)
#define STM32_RCC_APB1ENR1   (STM32_RCC_BASE + STM32_RCC_APB1ENR1_OFFSET)
#define STM32_RCC_APB1ENR2   (STM32_RCC_BASE + STM32_RCC_APB1ENR2_OFFSET)
#define STM32_RCC_APB1LPENR1 (STM32_RCC_BASE + STM32_RCC_APB1LPENR1_OFFSET)
#define STM32_RCC_APB2ENR    (STM32_RCC_BASE + STM32_RCC_APB2ENR_OFFSET)
#define STM32_RCC_APB2LPENR  (STM32_RCC_BASE + STM32_RCC_APB2LPENR_OFFSET)
#define STM32_RCC_APB4ENR1   (STM32_RCC_BASE + STM32_RCC_APB4ENR1_OFFSET)
#define STM32_RCC_APB4ENR2   (STM32_RCC_BASE + STM32_RCC_APB4ENR2_OFFSET)
#define STM32_RCC_APB4LPENR1 (STM32_RCC_BASE + STM32_RCC_APB4LPENR1_OFFSET)
#define STM32_RCC_APB5ENR    (STM32_RCC_BASE + STM32_RCC_APB5ENR_OFFSET)
#define STM32_RCC_APB5LPENR  (STM32_RCC_BASE + STM32_RCC_APB5LPENR_OFFSET)
#define STM32_RCC_APB5ENSR   (STM32_RCC_BASE + STM32_RCC_APB5ENSR_OFFSET)
#define STM32_RCC_APB5LPENSR (STM32_RCC_BASE + STM32_RCC_APB5LPENSR_OFFSET)

#define STM32_RCC_APB5RSTR   (STM32_RCC_BASE + STM32_RCC_APB5RSTR_OFFSET)
#define STM32_RCC_CCIPR1     (STM32_RCC_BASE + STM32_RCC_CCIPR1_OFFSET)

#define STM32_RCC_MEMENSR    (STM32_RCC_BASE + STM32_RCC_MEMENSR_OFFSET)
#define STM32_RCC_AHB2ENSR   (STM32_RCC_BASE + STM32_RCC_AHB2ENSR_OFFSET)
#define STM32_RCC_AHB4ENSR   (STM32_RCC_BASE + STM32_RCC_AHB4ENSR_OFFSET)
#define STM32_RCC_APB1ENSR1  (STM32_RCC_BASE + STM32_RCC_APB1ENSR1_OFFSET)
#define STM32_RCC_APB2ENSR   (STM32_RCC_BASE + STM32_RCC_APB2ENSR_OFFSET)
#define STM32_RCC_APB4ENSR1  (STM32_RCC_BASE + STM32_RCC_APB4ENSR1_OFFSET)
#define STM32_RCC_APB4ENSR2  (STM32_RCC_BASE + STM32_RCC_APB4ENSR2_OFFSET)
#define STM32_RCC_BUSLPENSR  (STM32_RCC_BASE + STM32_RCC_BUSLPENSR_OFFSET)
#define STM32_RCC_MEMLPENSR  (STM32_RCC_BASE + STM32_RCC_MEMLPENSR_OFFSET)
#define STM32_RCC_APB2LPENSR (STM32_RCC_BASE + STM32_RCC_APB2LPENSR_OFFSET)

#define STM32_RCC_CCR        (STM32_RCC_BASE + STM32_RCC_CCR_OFFSET)
#define STM32_RCC_APB2ENCR   (STM32_RCC_BASE + STM32_RCC_APB2ENCR_OFFSET)
#define STM32_RCC_CSR        (STM32_RCC_BASE + STM32_RCC_CSR_OFFSET)
#define STM32_RCC_HWRSR      (STM32_RCC_BASE + STM32_RCC_HWRSR_OFFSET)
#define STM32_RCC_RSR        (STM32_RCC_BASE + STM32_RCC_RSR_OFFSET)

/* Register Bitfield Definitions ********************************************/

/* Clock control register (CMSIS RCC_CR).  Ready flags are read via
 * STM32_RCC_SR; CR.xxxON bits are toggled through the CCR (clear) /
 * CSR (set) atomic aliases, not by a read-modify-write on CR itself.
 */

#define RCC_CR_LSION             (1 << 0)   /* LSI oscillator enable */
#define RCC_CR_HSION             (1 << 3)   /* HSI enable */
#define RCC_CR_HSEON             (1 << 4)   /* HSE enable */
#define RCC_CR_PLL1ON            (1 << 8)   /* PLL1 enable */
#define RCC_CR_PLL2ON            (1 << 9)   /* PLL2 enable */
#define RCC_CR_PLL3ON            (1 << 10)  /* PLL3 enable */
#define RCC_CR_PLL4ON            (1 << 11)  /* PLL4 enable */

/* Clock status register */

#define RCC_SR_LSIRDY            (1 << 0)   /* LSI ready flag */

/* HW reset status register (HWRSR): sticky reset-cause flags */

#define RCC_HWRSR_BORRSTF        (1 << 21)  /* BOR reset flag */
#define RCC_HWRSR_PINRSTF        (1 << 22)  /* Pin (NRST) reset flag */
#define RCC_HWRSR_PORRSTF        (1 << 23)  /* POR/PDR reset flag */
#define RCC_HWRSR_SFTRSTF        (1 << 24)  /* Software reset flag */
#define RCC_HWRSR_IWDGRSTF       (1 << 26)  /* IWDG reset flag */
#define RCC_HWRSR_WWDGRSTF       (1 << 28)  /* WWDG reset flag */
#define RCC_HWRSR_LPWRRSTF       (1 << 30)  /* Illegal Stop/Standby flag */

/* Reset status register (RSR): the CPU/application-visible sticky reset-
 * cause flags.  On STM32N6 the debug/CPU side reads its reset cause here
 * (HWRSR mirrors the hardware domain and reads 0 from the CPU AP).  Write
 * RMVF to clear the whole set so the next reset reports a fresh cause.
 */

#define RCC_RSR_RMVF             (1 << 16)  /* Remove reset flags */
#define RCC_RSR_BORRSTF          (1 << 21)  /* BOR reset flag */
#define RCC_RSR_PINRSTF          (1 << 22)  /* Pin (NRST) reset flag */
#define RCC_RSR_PORRSTF          (1 << 23)  /* POR/PDR reset flag */
#define RCC_RSR_SFTRSTF          (1 << 24)  /* Software reset flag */
#define RCC_RSR_IWDGRSTF         (1 << 26)  /* IWDG reset flag */
#define RCC_RSR_WWDGRSTF         (1 << 28)  /* WWDG reset flag */
#define RCC_RSR_LPWRRSTF         (1 << 30)  /* Illegal Stop/Standby flag */
#define RCC_SR_HSIRDY            (1 << 3)   /* HSI ready flag */
#define RCC_SR_HSERDY            (1 << 4)   /* HSE ready flag */
#define RCC_SR_PLL1RDY           (1 << 8)   /* PLL1 ready flag */
#define RCC_SR_PLL2RDY           (1 << 9)   /* PLL2 ready flag */
#define RCC_SR_PLL3RDY           (1 << 10)  /* PLL3 ready flag */
#define RCC_SR_PLL4RDY           (1 << 11)  /* PLL4 ready flag */

/* Kernel clock select 4: I2C1-4 clock source (CMSIS RCC_CCIPR4_I2CxSEL).
 * Encoding per ST LL: 0=PCLK1, 1=CLKP, 2=IC10, 3=IC15, 4=MSI, 5=HSI.
 * This port pins every I2C instance to HSI (64 MHz) so the TIMINGR
 * presets match a known kernel clock rather than the reset default
 * (PCLK1 at APB1 speed, which would make SCL 1.5-3x too fast).
 */

#define RCC_CCIPR4_I2C1SEL_SHIFT (0)
#define RCC_CCIPR4_I2C1SEL_MASK  (0x7 << RCC_CCIPR4_I2C1SEL_SHIFT)
#define RCC_CCIPR4_I2C1SEL_HSI   (0x5 << RCC_CCIPR4_I2C1SEL_SHIFT)

#define RCC_CCIPR4_I2C2SEL_SHIFT (4)
#define RCC_CCIPR4_I2C2SEL_MASK  (0x7 << RCC_CCIPR4_I2C2SEL_SHIFT)
#define RCC_CCIPR4_I2C2SEL_HSI   (0x5 << RCC_CCIPR4_I2C2SEL_SHIFT)

#define RCC_CCIPR4_I2C3SEL_SHIFT (8)
#define RCC_CCIPR4_I2C3SEL_MASK  (0x7 << RCC_CCIPR4_I2C3SEL_SHIFT)
#define RCC_CCIPR4_I2C3SEL_HSI   (0x5 << RCC_CCIPR4_I2C3SEL_SHIFT)

#define RCC_CCIPR4_I2C4SEL_SHIFT (12)
#define RCC_CCIPR4_I2C4SEL_MASK  (0x7 << RCC_CCIPR4_I2C4SEL_SHIFT)
#define RCC_CCIPR4_I2C4SEL_HSI   (0x5 << RCC_CCIPR4_I2C4SEL_SHIFT)

/* Kernel clock select 4: LTDC kernel clock source (CMSIS RCC_CCIPR4_LTDCSEL).
 * 0b00=PCLK5, 0b01=CLKP, 0b10=IC16, 0b11=HSI. */

#define RCC_CCIPR4_LTDCSEL_SHIFT (24)
#define RCC_CCIPR4_LTDCSEL_MASK  (0x3 << RCC_CCIPR4_LTDCSEL_SHIFT)
#define RCC_CCIPR4_LTDCSEL_IC16  (0x2 << RCC_CCIPR4_LTDCSEL_SHIFT)

/* Kernel clock select 7: RTC clock source (CMSIS RCC_CCIPR7_RTCSEL,
 * bits 9:8).  Encoding per ST HAL: 0=no clock, 1=LSE, 2=LSI, 3=HSE/div.
 */

#define RCC_CCIPR7_RTCSEL_SHIFT  (8)
#define RCC_CCIPR7_RTCSEL_MASK   (0x3 << RCC_CCIPR7_RTCSEL_SHIFT)
#define RCC_CCIPR7_RTCSEL_LSI    (0x2 << RCC_CCIPR7_RTCSEL_SHIFT)

/* Kernel clock select 12: LPTIM1 clock source (CMSIS RCC_CCIPR12_LPTIM1SEL,
 * bits 10:8).  Encoding per ST LL: 0=PCLK1, 1=CLKP, 2=LSE, 4=LSI.
 */

#define RCC_CCIPR12_LPTIM1SEL_SHIFT (8)
#define RCC_CCIPR12_LPTIM1SEL_MASK  (0x7 << RCC_CCIPR12_LPTIM1SEL_SHIFT)
#define RCC_CCIPR12_LPTIM1SEL_LSI   (0x4 << RCC_CCIPR12_LPTIM1SEL_SHIFT)

/* Kernel clock select 12 also carries LPTIM2-5 (CMSIS
 * RCC_CCIPR12_LPTIMnSEL: LPTIM2 bits 14:12, LPTIM3 18:16, LPTIM4 22:20,
 * LPTIM5 26:24).  Encoding per RM: 0=PCLK4, 3=LSE, 4=LSI.  All four
 * select LSI here to match LPTIM1.
 */

#define RCC_CCIPR12_LPTIM2SEL_SHIFT (12)
#define RCC_CCIPR12_LPTIM2SEL_MASK  (0x7 << RCC_CCIPR12_LPTIM2SEL_SHIFT)
#define RCC_CCIPR12_LPTIM2SEL_LSI   (0x4 << RCC_CCIPR12_LPTIM2SEL_SHIFT)

#define RCC_CCIPR12_LPTIM3SEL_SHIFT (16)
#define RCC_CCIPR12_LPTIM3SEL_MASK  (0x7 << RCC_CCIPR12_LPTIM3SEL_SHIFT)
#define RCC_CCIPR12_LPTIM3SEL_LSI   (0x4 << RCC_CCIPR12_LPTIM3SEL_SHIFT)

#define RCC_CCIPR12_LPTIM4SEL_SHIFT (20)
#define RCC_CCIPR12_LPTIM4SEL_MASK  (0x7 << RCC_CCIPR12_LPTIM4SEL_SHIFT)
#define RCC_CCIPR12_LPTIM4SEL_LSI   (0x4 << RCC_CCIPR12_LPTIM4SEL_SHIFT)

#define RCC_CCIPR12_LPTIM5SEL_SHIFT (24)
#define RCC_CCIPR12_LPTIM5SEL_MASK  (0x7 << RCC_CCIPR12_LPTIM5SEL_SHIFT)
#define RCC_CCIPR12_LPTIM5SEL_LSI   (0x4 << RCC_CCIPR12_LPTIM5SEL_SHIFT)

/* Clock configuration register 1.
 *
 * IMPORTANT (matches upstream NuttX STM32N6 port comment, verified
 * against ST clock-tree behavior): CFGR1 latches after its first
 * write following reset -- CPUSW and SYSSW MUST be written together
 * in a single putreg32() call, and a second write to CFGR1 after the
 * switch has taken effect can hang/crash the part (SRAM clock domain
 * drops).  Callers must check CPUSWS/SYSSWS before attempting to
 * rewrite CFGR1 (see stm32n6_clockconfig()).
 *
 * SYSSW/SYSSWS = 0b11 selects a group of three IC dividers (IC2 for
 * SYSCLK domain A, IC6 for domain B, IC11 for domain C); the SVD/CMSIS
 * naming exposes this as a single 2-bit mux value even though three
 * ICs are actually engaged together.
 */

#define RCC_CFGR1_SYSSWS_SHIFT         (28)
#define RCC_CFGR1_SYSSWS_MASK          (0x3 << RCC_CFGR1_SYSSWS_SHIFT)
#define RCC_CFGR1_SYSSWS_IC2_IC6_IC11  (3 << RCC_CFGR1_SYSSWS_SHIFT)

#define RCC_CFGR1_SYSSW_SHIFT          (24)
#define RCC_CFGR1_SYSSW_MASK           (0x3 << RCC_CFGR1_SYSSW_SHIFT)
#define RCC_CFGR1_SYSSW_IC2_IC6_IC11   (3 << RCC_CFGR1_SYSSW_SHIFT)

#define RCC_CFGR1_CPUSWS_SHIFT         (20)
#define RCC_CFGR1_CPUSWS_MASK          (0x3 << RCC_CFGR1_CPUSWS_SHIFT)
#define RCC_CFGR1_CPUSWS_IC1           (3 << RCC_CFGR1_CPUSWS_SHIFT)

#define RCC_CFGR1_CPUSW_SHIFT          (16)
#define RCC_CFGR1_CPUSW_MASK           (0x3 << RCC_CFGR1_CPUSW_SHIFT)
#define RCC_CFGR1_CPUSW_IC1            (3 << RCC_CFGR1_CPUSW_SHIFT)

/* Clock configuration register 2.  HPRE divides the SYSCLK domain fed
 * to the AHB/APB bus matrix.
 */

#define RCC_CFGR2_HPRE_SHIFT      (20)
#define RCC_CFGR2_HPRE_MASK       (0x7 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLK     (0 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLKd2   (1 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLKd4   (2 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLKd8   (3 << RCC_CFGR2_HPRE_SHIFT)
#define RCC_CFGR2_HPRE_SYSCLKd16  (4 << RCC_CFGR2_HPRE_SHIFT)

/* PLL1 configuration register 1.  SEL chooses the PLL1 reference
 * clock, DIVM is the reference (input) divider, DIVN is the feedback
 * (multiplier) divider -- all three fields share this single
 * register; there is no independent DIVN register in the
 * configuration sequence.
 */

#define RCC_PLL1CFGR1_SEL_SHIFT   (28)
#define RCC_PLL1CFGR1_SEL_MASK    (0x7 << RCC_PLL1CFGR1_SEL_SHIFT)
#define RCC_PLL1CFGR1_SEL_HSI     (0 << RCC_PLL1CFGR1_SEL_SHIFT)
#define RCC_PLL1CFGR1_SEL_HSE     (1 << RCC_PLL1CFGR1_SEL_SHIFT)

#define RCC_PLL1CFGR1_DIVM_SHIFT  (20)  /* Bits 25-20: reference divider */
#define RCC_PLL1CFGR1_DIVM_MASK   (0x3f << RCC_PLL1CFGR1_DIVM_SHIFT)

#define RCC_PLL1CFGR1_DIVN_SHIFT  (8)   /* Bits 19-8: feedback divider */
#define RCC_PLL1CFGR1_DIVN_MASK   (0xfff << RCC_PLL1CFGR1_DIVN_SHIFT)

/* PLL1 configuration register 3: post-dividers and modulation control */

#define RCC_PLL1CFGR3_PDIVEN      (1 << 30)  /* Post-divider/output enable */

#define RCC_PLL1CFGR3_PDIV1_SHIFT (27)  /* Bits 29-27: post-divider 1 */
#define RCC_PLL1CFGR3_PDIV1_MASK  (0x7 << RCC_PLL1CFGR3_PDIV1_SHIFT)

#define RCC_PLL1CFGR3_PDIV2_SHIFT (24)  /* Bits 26-24: post-divider 2 */
#define RCC_PLL1CFGR3_PDIV2_MASK  (0x7 << RCC_PLL1CFGR3_PDIV2_SHIFT)

#define RCC_PLL1CFGR3_MODSSDIS    (1 << 2)  /* Modulation spread-spectrum
                                              * disable */

/* IC1..IC20 configuration registers -- all share the same layout.
 * SEL chooses the PLLn source (PLL1..PLL4); INT is an 8-bit integer
 * divider field where INT[7:0] = N-1 for a divide ratio of N.
 */

#define RCC_ICCFGR_SEL_SHIFT      (28)
#define RCC_ICCFGR_SEL_MASK       (0x3 << RCC_ICCFGR_SEL_SHIFT)
#define RCC_ICCFGR_SEL_PLL1       (0 << RCC_ICCFGR_SEL_SHIFT)
#define RCC_ICCFGR_SEL_PLL2       (1 << RCC_ICCFGR_SEL_SHIFT)

#define RCC_ICCFGR_INT_SHIFT      (16)
#define RCC_ICCFGR_INT_MASK       (0xff << RCC_ICCFGR_INT_SHIFT)

/* IC divider enable register */

#define RCC_DIVENR_IC1EN          (1 << 0)
#define RCC_DIVENR_IC2EN          (1 << 1)
#define RCC_DIVENR_IC3EN          (1 << 2)
#define RCC_DIVENR_IC6EN          (1 << 5)
#define RCC_DIVENR_IC11EN         (1 << 10)

/* SRAM clock enable register.  The boot ROM only enables AXISRAM1/2;
 * the NuttX heap spans additional banks and needs the rest enabled
 * explicitly (and re-armed after the CFGR1 clock-domain switch, since
 * some SRAM bank clocks can drop out across that transition).
 */

#define RCC_MEMENR_CACHEAXIRAMEN  (1 << 10)
#define RCC_MEMENR_AXISRAM2EN     (1 << 8)
#define RCC_MEMENR_AXISRAM1EN     (1 << 7)
#define RCC_MEMENR_AXISRAM6EN     (1 << 3)
#define RCC_MEMENR_AXISRAM5EN     (1 << 2)
#define RCC_MEMENR_AXISRAM4EN     (1 << 1)
#define RCC_MEMENR_AXISRAM3EN     (1 << 0)
#define RCC_MEMENR_ALLAXISRAM     (RCC_MEMENR_AXISRAM1EN | \
                                    RCC_MEMENR_AXISRAM2EN | \
                                    RCC_MEMENR_AXISRAM3EN | \
                                    RCC_MEMENR_AXISRAM4EN | \
                                    RCC_MEMENR_AXISRAM5EN | \
                                    RCC_MEMENR_AXISRAM6EN)

/* RCC_MEMRSTR: one reset bit per embedded memory (RM0486 14.10.63/131).
 * The clear alias (RCC_MEMRSTCR) releases a bank from reset.
 */

#define RCC_MEMRSTR_AXISRAM3RST       (1 << 0)
#define RCC_MEMRSTR_AXISRAM4RST       (1 << 1)
#define RCC_MEMRSTR_AXISRAM5RST       (1 << 2)
#define RCC_MEMRSTR_AXISRAM6RST       (1 << 3)
#define RCC_MEMRSTR_AXISRAM1RST       (1 << 7)
#define RCC_MEMRSTR_AXISRAM2RST       (1 << 8)
#define RCC_MEMRSTR_FLEXRAMRST        (1 << 9)
#define RCC_MEMRSTR_CACHEAXIRAMRST    (1 << 10)
#define RCC_MEMRSTR_ALLNPURAM         (RCC_MEMRSTR_AXISRAM3RST | \
                                       RCC_MEMRSTR_AXISRAM4RST | \
                                       RCC_MEMRSTR_AXISRAM5RST | \
                                       RCC_MEMRSTR_AXISRAM6RST)

/* RCC_BUSENR: embedded bus clocks (RM0486 14.10.76).  ACLKNEN gates
 * ck_icn_npu; RM0486 14.4: "If the clock is disabled, the NPU cannot work
 * (no interconnect downstream), and the CPU cannot access AXISRAM3/4/5/6,
 * the CACHEAXI RAM, or the FLEXRAM."
 */

#define RCC_BUSENR_ACLKNEN            (1 << 0)
#define RCC_BUSENR_ACLKNCEN           (1 << 1)

/* RCC_AHB2ENR bit 12: RAMCFG clock (RM0486 14.10.80). */

#define RCC_AHB2ENR_RAMCFGEN          (1 << 12)

/* AHB4ENR bits: GPIO port + PWR enables (CMSIS-verified positions;
 * note GPION/O/P/Q are NOT contiguous with GPIOA-H).
 */

#define RCC_AHB4ENR_GPIOAEN      (1 << 0)
#define RCC_AHB4ENR_GPIOBEN      (1 << 1)
#define RCC_AHB4ENR_GPIOCEN      (1 << 2)
#define RCC_AHB4ENR_GPIODEN      (1 << 3)
#define RCC_AHB4ENR_GPIOEEN      (1 << 4)
#define RCC_AHB4ENR_GPIOFEN      (1 << 5)
#define RCC_AHB4ENR_GPIOGEN      (1 << 6)
#define RCC_AHB4ENR_GPIOHEN      (1 << 7)
#define RCC_AHB4ENR_GPIONEN      (1 << 13)
#define RCC_AHB4ENR_GPIOOEN      (1 << 14)
#define RCC_AHB4ENR_GPIOPEN      (1 << 15)
#define RCC_AHB4ENR_GPIOQEN      (1 << 16)
#define RCC_AHB4ENR_PWREN        (1 << 18)

/* APB2ENR bits: USART1/6, UART9, USART10 enables */

#define RCC_APB2ENR_USART1EN     (1 << 4)
#define RCC_APB2ENR_USART6EN     (1 << 5)
#define RCC_APB2ENR_UART9EN      (1 << 7)
#define RCC_APB2ENR_USART10EN    (1 << 8)
#define RCC_APB2ENR_TIM15EN      (1 << 16)

/* AHB3ENR bits: RNG, HASH, RIFSC enable */

#define RCC_AHB3ENR_RNGEN        (1 << 0)
#define RCC_AHB3ENR_HASHEN       (1 << 1)
#define RCC_AHB3ENR_RIFSCEN      (1 << 9)

/* AHB1ENR bits: GPDMA1, ADC12 enable */

#define RCC_AHB1ENR_GPDMA1EN     (1 << 4)
#define RCC_AHB1ENR_ADC12EN      (1 << 5)

/* APB1LPENR1 bits: keep the peripheral clock running through CPU Sleep
 * (WFI).  Without the matching LPEN bit an APB1 peripheral's clock gates
 * while the core idles, so its counter freezes and never raises an update
 * interrupt to wake the CPU.
 */

#define RCC_APB1LPENR1_TIM2LPEN  (1 << 0)
#define RCC_APB1LPENR1_TIM3LPEN  (1 << 1)
#define RCC_APB1LPENR1_TIM5LPEN  (1 << 3)
#define RCC_APB1LPENR1_LPTIM1LPEN (1 << 9)

/* APB1ENR1 bits: peripheral enables */

#define RCC_APB1ENR1_TIM2EN      (1 << 0)
#define RCC_APB1ENR1_TIM3EN      (1 << 1)
#define RCC_APB1ENR1_TIM5EN      (1 << 3)
#define RCC_APB1ENR1_LPTIM1EN    (1 << 9)
#define RCC_APB1ENR1_WWDGEN      (1 << 11)
#define RCC_APB1ENR1_USART2EN    (1 << 17)
#define RCC_APB1ENR1_USART3EN    (1 << 18)
#define RCC_APB1ENR1_UART4EN     (1 << 19)
#define RCC_APB1ENR1_UART5EN     (1 << 20)
#define RCC_APB1ENR1_I2C1EN      (1 << 21)
#define RCC_APB1ENR1_I2C2EN      (1 << 22)

/* AHB5ENR bits: DMA2D/XSPI/SDMMC/GPU2D enables (positions per CMSIS
 * stm32n647xx.h).  The XSPI/SDMMC symbols are currently unreferenced —
 * their clocks are left enabled by the ROM/FSBL in DEV boot — but the
 * positions are corrected here so a future clock-gating path uses the
 * right bits.
 */

#define RCC_AHB5ENR_DMA2DEN      (1 << 1)
#define RCC_AHB5ENR_XSPI1EN      (1 << 5)
#define RCC_AHB5ENR_SDMMC2EN     (1 << 7)
#define RCC_AHB5ENR_SDMMC1EN     (1 << 8)
#define RCC_AHB5ENR_XSPI2EN      (1 << 12)
#define RCC_AHB5ENR_XSPIMEN      (1 << 13)
#define RCC_AHB5ENR_GPU2DEN      (1 << 20)
#define RCC_AHB5ENR_NPUEN       (1 << 31)   /* NPU clock enable */
#define RCC_AHB5ENR_CACHEAXIEN  (1 << 30)   /* NPU cache RAM clock enable */

/* AHB5RSTR bits: NPU reset (CMSIS RCC_AHB5RSTR_NPURST, bit 31). */

#define RCC_AHB5RSTR_NPURST     (1 << 31)   /* NPU reset */
#define RCC_AHB5RSTR_CACHEAXIRST (1 << 30)  /* NPU cache RAM reset */

/* APB4ENR1 bits: RTC enable (CMSIS RCC_APB4ENR1_RTCEN, bit 16).
 * IWDG has no software clock-gating enable bit on STM32N6 (neither
 * CMSIS nor upstream NuttX define an RCC_*ENR*_IWDGEN); the watchdog
 * clock is always on once the IWDG is started.
 */

#define RCC_APB4ENR1_I2C4EN      (1 << 7)
#define RCC_APB4ENR1_LPTIM2EN    (1 << 9)
#define RCC_APB4ENR1_LPTIM3EN    (1 << 10)
#define RCC_APB4ENR1_LPTIM4EN    (1 << 11)
#define RCC_APB4ENR1_LPTIM5EN    (1 << 12)
#define RCC_APB4ENR1_RTCEN       (1 << 16)

/* APB4LPENR1 bits: keep LPTIM2-5 clocked through CPU Sleep (WFI), mirroring
 * the LPTIM1 keep-alive on APB1 (CMSIS RCC_APB4LPENR1_LPTIMnLPEN).
 */

#define RCC_APB4LPENR1_LPTIM2LPEN (1 << 9)
#define RCC_APB4LPENR1_LPTIM3LPEN (1 << 10)
#define RCC_APB4LPENR1_LPTIM4LPEN (1 << 11)
#define RCC_APB4LPENR1_LPTIM5LPEN (1 << 12)

/* APB4ENR2 bits (CMSIS RCC_APB4ENR2_*).  SYSCFGEN gates the SYSCFG
 * block used for the ES0620 I/O-compensation writes; BSECEN must stay
 * set or WFI/sleep fails (ES0620).  Written via the APB4ENSR2 set
 * alias.
 */

#define RCC_APB4ENR2_SYSCFGEN    (1 << 0)
#define RCC_APB4ENR2_BSECEN      (1 << 1)
#define RCC_APB4ENR2_DTSEN       (1 << 2)

/* BUSLPENR bits: keep the AXI-node bus clocks running through CSLEEP
 * (WFI).  Without these the AXISRAM banks lose their bus clock during
 * WFI and the core never wakes.  Written via the BUSLPENSR set alias.
 */

#define RCC_BUSLPENR_ACLKNLPEN   (1 << 0)
#define RCC_BUSLPENR_ACLKNCLPEN  (1 << 1)

/* MEMLPENR bits: keep the AXISRAM banks (and the cache-backing AXIRAM)
 * clocked through CSLEEP (WFI).  Mirrors the MEMENR layout above but
 * for the low-power (sleep) clock gate.  Written via MEMLPENSR.
 */

/* RM0486 14.10.93 / CMSIS stm32n647xx.h RCC_MEMLPENR (memories sleep enable):
 * bit12 BOOTROMLPEN bit11 VENCRAMLPEN bit10 CACHEAXIRAMLPEN bit9 FLEXRAMLPEN
 * bit8 AXISRAM2LPEN bit7 AXISRAM1LPEN bit6 BKPSRAMLPEN bit5 AHBSRAM2LPEN
 * bit4 AHBSRAM1LPEN bit3 AXISRAM6LPEN bit2 AXISRAM5LPEN bit1 AXISRAM4LPEN
 * bit0 AXISRAM3LPEN */
#define RCC_MEMLPENR_BOOTROMLPEN    (1 << 12)
#define RCC_MEMLPENR_VENCRAMLPEN    (1 << 11)
#define RCC_MEMLPENR_CACHEAXIRAMLPEN (1 << 10)
#define RCC_MEMLPENR_FLEXRAMLPEN    (1 << 9)
#define RCC_MEMLPENR_AXISRAM2LPEN   (1 << 8)
#define RCC_MEMLPENR_AXISRAM1LPEN   (1 << 7)
#define RCC_MEMLPENR_BKPSRAMLPEN    (1 << 6)
#define RCC_MEMLPENR_AHBSRAM2LPEN   (1 << 5)
#define RCC_MEMLPENR_AHBSRAM1LPEN   (1 << 4)
#define RCC_MEMLPENR_AXISRAM6LPEN   (1 << 3)
#define RCC_MEMLPENR_AXISRAM5LPEN   (1 << 2)
#define RCC_MEMLPENR_AXISRAM4LPEN   (1 << 1)
#define RCC_MEMLPENR_AXISRAM3LPEN   (1 << 0)
#define RCC_MEMLPENR_ALLAXISRAM      (RCC_MEMLPENR_FLEXRAMLPEN | \
                                      RCC_MEMLPENR_AXISRAM1LPEN | \
                                      RCC_MEMLPENR_AXISRAM2LPEN | \
                                      RCC_MEMLPENR_AXISRAM3LPEN | \
                                      RCC_MEMLPENR_AXISRAM4LPEN | \
                                      RCC_MEMLPENR_AXISRAM5LPEN | \
                                      RCC_MEMLPENR_AXISRAM6LPEN | \
                                      RCC_MEMLPENR_AHBSRAM1LPEN | \
                                      RCC_MEMLPENR_AHBSRAM2LPEN | \
                                      RCC_MEMLPENR_BKPSRAMLPEN)

/* APB2LPENR bits: keep USART1 clocked through CSLEEP so the console
 * survives WFI.  Written via the APB2LPENSR set alias.
 */

#define RCC_APB2LPENR_USART1LPEN (1 << 4)
#define RCC_APB2LPENR_TIM15LPEN  (1 << 16)

/* CCIPR13: USART1 kernel clock source select (bits 0-2).  Value 6
 * selects HSI, matching CMSIS RCC_CCIPR13_USART1SEL and the value the
 * upstream NuttX port uses so BRR stays independent of SYSCLK.
 */

#define RCC_CCIPR13_USART1SEL_SHIFT  (0)
#define RCC_CCIPR13_USART1SEL_MASK   (0x7 << RCC_CCIPR13_USART1SEL_SHIFT)
#define RCC_CCIPR13_USART1SEL_HSI    (6 << RCC_CCIPR13_USART1SEL_SHIFT)

/* APB5ENR bits: LTDC (and DCMIPP) live on APB5.  Written via APB5ENSR. */

#define RCC_APB5ENR_LTDCEN      (1 << 1)
#define RCC_APB5ENR_DCMIPPEN    (1 << 2)
#define RCC_APB5ENR_CSIEN       (1 << 6)
#define RCC_APB5LPENR_LTDCLPEN    (1 << 1)  /* keep LTDC APB5 clock on in Sleep */
#define RCC_APB5LPENR_DCMIPPLPEN  (1 << 2)  /* keep DCMIPP APB5 clock on in Sleep */
#define RCC_APB5LPENR_CSILPEN     (1 << 6)  /* keep CSI APB5 clock on in Sleep */

/* APB5RSTR bits: reset DCMIPP and CSI-2 host (write via APB5RSTR directly) */

#define RCC_APB5RSTR_DCMIPPRST  (1 << 2)
#define RCC_APB5RSTR_CSIRST     (1 << 6)

/* DIVENR bits: IC divider enables.  IC16 feeds the LTDC kernel clock,
 * IC17 the DCMIPP kernel clock, IC18 the CSI-2 host clock.
 */

#define RCC_DIVENR_IC16EN       (1 << 15)
#define RCC_DIVENR_IC17EN       (1 << 16)
#define RCC_DIVENR_IC18EN       (1 << 17)

/* IC16CFGR bits: LTDC kernel clock via IC16.
 *   IC16SEL: source select (0 = PLL1, matching bare-metal 15_RGBLCD)
 *   IC16INT: integer divider (value written = divider - 1)
 */

#define RCC_IC16CFGR_IC16INT_SHIFT  (16)
#define RCC_IC16CFGR_IC16INT_MASK   (0xff << RCC_IC16CFGR_IC16INT_SHIFT)
#define RCC_IC16CFGR_IC16SEL_SHIFT  (28)
#define RCC_IC16CFGR_IC16SEL_MASK   (0x3 << RCC_IC16CFGR_IC16SEL_SHIFT)
#define RCC_IC16CFGR_IC16SEL_PLL1   (0)

/* IC17CFGR / IC18CFGR bits: same layout as IC16CFGR (SEL bits 28-29,
 * INT bits 16-23).  PLL1 is source value 1 (0x1 << 28).
 */

#define RCC_IC17CFGR_IC17INT_SHIFT  (16)
#define RCC_IC17CFGR_IC17INT_MASK   (0xff << RCC_IC17CFGR_IC17INT_SHIFT)
#define RCC_IC17CFGR_IC17SEL_SHIFT  (28)
#define RCC_IC17CFGR_IC17SEL_MASK   (0x3 << RCC_IC17CFGR_IC17SEL_SHIFT)
#define RCC_IC17CFGR_IC17SEL_PLL1   (0x0 << RCC_IC17CFGR_IC17SEL_SHIFT)

#define RCC_IC18CFGR_IC18INT_SHIFT  (16)
#define RCC_IC18CFGR_IC18INT_MASK   (0xff << RCC_IC18CFGR_IC18INT_SHIFT)
#define RCC_IC18CFGR_IC18SEL_SHIFT  (28)
#define RCC_IC18CFGR_IC18SEL_MASK   (0x3 << RCC_IC18CFGR_IC18SEL_SHIFT)
#define RCC_IC18CFGR_IC18SEL_PLL1   (0x0 << RCC_IC18CFGR_IC18SEL_SHIFT)

/* CCIPR1 bits: DCMIPP kernel clock source selection.
 *   0 = PCLK5, 1 = CLKP, 2 = IC17
 */

#define RCC_CCIPR1_DCMIPPSEL_SHIFT  (20)
#define RCC_CCIPR1_DCMIPPSEL_MASK   (0x3 << RCC_CCIPR1_DCMIPPSEL_SHIFT)
#define RCC_CCIPR1_DCMIPPSEL_PCLK5  (0)
#define RCC_CCIPR1_DCMIPPSEL_IC17   (0x2 << RCC_CCIPR1_DCMIPPSEL_SHIFT)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_RCC_H */
