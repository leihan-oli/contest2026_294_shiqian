/****************************************************************************
 * board/atk-dnn647/src/stm32n6_clockconfig.c
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
 * EdgeSight - STM32N6 clock configuration for 800MHz operation.
 *
 * Reference: ST VENC_RTSP_Server SystemClock_Config_800MHz.c
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

#ifdef CONFIG_EDGESIGHT_CLOCK_800MHZ

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* STM32N6 RCC register base */

#define STM32_RCC_BASE         0x44020C00

/* PLL configuration registers (offsets from RCC base) */

#define RCC_PLL1CFGR1          0x040
#define RCC_PLL1CFGR2          0x044
#define RCC_PLL1CFGR3          0x048
#define RCC_PLL1CFGR4          0x04C
#define RCC_PLL2CFGR1          0x058
#define RCC_PLL2CFGR2          0x05C
#define RCC_PLL2CFGR3          0x060
#define RCC_PLL2CFGR4          0x064
#define RCC_PLL4CFGR1          0x088
#define RCC_PLL4CFGR2          0x08C
#define RCC_PLL4CFGR3          0x090
#define RCC_PLL4CFGR4          0x094

/* Interconnect clock registers */

#define RCC_IC1CFGR            0x100
#define RCC_IC2CFGR            0x104
#define RCC_IC4CFGR            0x10C
#define RCC_IC6CFGR            0x114
#define RCC_IC11CFGR           0x128

/* Clock source selection */

#define RCC_CCIPR1             0x1A0

/* Bus clock dividers */

#define RCC_CFGR1              0x01C
#define RCC_CFGR2              0x020

/* PWR registers for voltage scaling */

#define STM32_PWR_BASE         0x44024800
#define PWR_VOSCR              0x010
#define PWR_VOSSR              0x014

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void putreg32(uint32_t val, uint32_t addr)
{
  *(volatile uint32_t *)addr = val;
}

static inline uint32_t getreg32(uint32_t addr)
{
  return *(volatile uint32_t *)addr;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/**
 * @brief Configure system clocks for 800MHz CPU operation
 *
 * Prerequisites:
 *   - SMPS must be in overdrive mode (board-specific)
 *   - Voltage regulator must be at Scale 0
 *
 * Clock tree after configuration:
 *   PLL1: HSI(64) / 2 * 25 = 800 MHz
 *     IC1 /1 = 800 MHz -> CPU
 *     IC2 /2 = 400 MHz -> AXI
 *     IC4 /4 = 200 MHz -> SDMMC
 *   PLL2: HSI(64) / 8 * 125 = 1000 MHz
 *     IC6 /1 = 1000 MHz -> NPU
 *   AHB /2 = 200 MHz -> HCLK
 *   APBx /1 = 200 MHz -> PCLKx
 *
 * NOTE: This function is a skeleton. Exact register bit fields
 * need validation against RM0486 reference manual on real HW.
 * The ST HAL equivalent is SystemClock_Config_800MHz.c which
 * uses HAL_RCC_OscConfig + HAL_RCC_ClockConfig. We document
 * the register-level approach for NuttX native driver.
 */

void stm32n6_clockconfig_800mhz(void)
{
  /* Step 1: Set voltage scaling to Scale 0 (highest perf)
   *
   * On real hardware via HAL:
   *   HAL_PWREx_ControlVoltageScaling(
   *     PWR_REGULATOR_VOLTAGE_SCALE0);
   *
   * Register level:
   *   Set PWR_VOSCR.VOS = 0b11
   *   Wait for PWR_VOSSR.VOSRDY
   */

  /* TODO: Implement voltage scaling */

  /* Step 2: Switch CPU/SYS to HSI before reconfiguring PLLs
   *
   * Set CFGR1.CPUSW = HSI
   * Set CFGR1.SYSSW = HSI
   * Wait for CFGR1.CPUSWRDY / SYSSWRDY
   */

  /* TODO: Switch to HSI */

  /* Step 3: Configure PLL1 = HSI(64) / M=2 * N=25 = 800MHz
   *
   * PLL1CFGR1: PLLSRC = HSI, PLLM = 2
   * PLL1CFGR2: PLLN = 25, PLLFractional = 0
   * PLL1CFGR3: PLLP1 = 1, PLLP2 = 1
   * PLL1CFGR4: PLLEN = 1
   * Wait for PLL1 ready
   */

  /* TODO: Configure PLL1 */

  /* Step 4: Configure PLL2 = HSI(64) / M=8 * N=125 = 1000MHz
   *
   * PLL2CFGR1: PLLSRC = HSI, PLLM = 8
   * PLL2CFGR2: PLLN = 125
   * PLL2CFGR3: PLLP1 = 1, PLLP2 = 1
   * PLL2CFGR4: PLLEN = 1
   * Wait for PLL2 ready
   */

  /* TODO: Configure PLL2 */

  /* Step 5: Configure PLL4 = HSI(64) / M=32 * N=40 = 80MHz
   *
   * PLL4CFGR1: PLLSRC = HSI, PLLM = 32
   * PLL4CFGR2: PLLN = 40
   * PLL4CFGR3: PLLP1 = 1, PLLP2 = 1
   * PLL4CFGR4: PLLEN = 1
   * Wait for PLL4 ready
   */

  /* TODO: Configure PLL4 */

  /* Step 6: Configure interconnect clocks
   *
   * IC1 (CPU):  PLL1 / 1 = 800 MHz
   * IC2 (AXI):  PLL1 / 2 = 400 MHz
   * IC4 (SDMMC): PLL1 / 4 = 200 MHz
   * IC6 (NPU):  PLL2 / 1 = 1000 MHz
   * IC11 (SRAM): PLL1 / 2 = 400 MHz
   */

  /* TODO: Configure IC dividers */

  /* Step 7: Set bus clock dividers
   *
   * HPRE  = /2  -> HCLK = 200 MHz
   * PPRE1 = /1  -> PCLK1 = 200 MHz
   * PPRE2 = /1  -> PCLK2 = 200 MHz
   * PPRE4 = /1  -> PCLK4 = 200 MHz
   * PPRE5 = /1  -> PCLK5 = 200 MHz
   */

  /* TODO: Configure bus dividers */

  /* Step 8: Switch CPU/SYS source to IC1/IC2
   *
   * Set CFGR1.CPUSW = IC1
   * Set CFGR1.SYSSW = IC2_IC6_IC11
   * Wait for ready
   */

  /* TODO: Switch clock sources */
}

#endif /* CONFIG_EDGESIGHT_CLOCK_800MHZ */
