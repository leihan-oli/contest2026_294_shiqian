/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_rcc.c
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
 * PLL1/IC clock-tree configuration is ported from the upstream Apache
 * NuttX STM32N6 port (arch/arm/src/stm32n6/stm32n6xx_rcc.c,
 * stm32_stdclockconfig()), which targets STM32N657 -- a part
 * confirmed (via CMSIS RCC_TypeDef diff) to share the identical RCC
 * IP with STM32N647.  See docs/adr/ADR-005.md for the clock-tree
 * architecture rationale.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>

#include "arm_internal.h"
#include "stm32n6_rcc.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Default clock frequencies (HSI mode, no PLL) */

#define STM32_HSI_FREQUENCY      64000000ul  /* 64 MHz internal RC */

/* Timeout for clock ready flags (100ms at ~64MHz loop rate) */

#define CLOCK_READY_TIMEOUT     (100 * CONFIG_BOARD_LOOPSPERMSEC)

/* PLL1/IC divider parameters come from board.h (STM32_PLL1_M,
 * STM32_PLL1_N, STM32_PLL1_IC1_DIV).  Fall back to a conservative
 * HSI-only default (no multiplication) if board.h has not defined
 * them, so this file still compiles standalone.
 */

#ifndef STM32_PLL1_M
#  define STM32_PLL1_M        1
#endif

#ifndef STM32_PLL1_N
#  define STM32_PLL1_N        1
#endif

#ifndef STM32_PLL1_IC1_DIV
#  define STM32_PLL1_IC1_DIV  1
#endif

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void rcc_enablehsi(void)
{
  uint32_t regval;

  /* Set HSION in CR (bit 3) */

  regval  = getreg32(STM32_RCC_CR);
  regval |= RCC_CR_HSION;
  putreg32(regval, STM32_RCC_CR);

  /* Wait for HSIRDY in SR (bit 3) */

  while ((getreg32(STM32_RCC_SR) & RCC_SR_HSIRDY) == 0)
    {
    }
}

#ifdef CONFIG_STM32N6_USE_HSE
static inline int rcc_enablehse(void)
{
  uint32_t regval;
  volatile int timeout;

  /* Set HSEON in CR (bit 4) */

  regval  = getreg32(STM32_RCC_CR);
  regval |= RCC_CR_HSEON;
  putreg32(regval, STM32_RCC_CR);

  /* Wait for HSERDY in SR (bit 4) with timeout */

  timeout = CLOCK_READY_TIMEOUT;

  while ((getreg32(STM32_RCC_SR) & RCC_SR_HSERDY) == 0)
    {
      if (--timeout <= 0)
        {
          return -1;
        }
    }

  return 0;
}
#endif

#ifdef CONFIG_STM32N6_USE_PLL1

/****************************************************************************
 * Name: rcc_configpll1
 *
 * Description:
 *   Configure and switch to PLL1 -> IC1/IC2/IC6/IC11 following the
 *   register-level sequence used by upstream NuttX
 *   stm32_stdclockconfig() (STM32N6 port).  This chip's clock-domain
 *   switch fabric differs fundamentally from the legacy STM32Fx/Hx
 *   PLL+prescaler model:
 *
 *     - PLL1CFGR1 packs SEL (reference source), DIVM (input divider),
 *       and DIVN (feedback divider) into a single register -- there
 *       is no separate PLL1CFGR2 multiplier register in this
 *       sequence.
 *     - PLL1 is enabled/disabled via the CSR (set) / CCR (clear)
 *       atomic alias registers, not by a read-modify-write on CR.
 *     - The VCO output feeds a bank of IC (Interconnect) dividers;
 *       CPUCLK comes from IC1, while SYSCLK is fed by three ICs
 *       (IC2/IC6/IC11) selected together as a single CFGR1.SYSSW
 *       group value.
 *     - CFGR1 latches after its first post-reset write: CPUSW and
 *       SYSSW MUST be written together in one putreg32(), and this
 *       function must not be called a second time after the switch
 *       has taken effect (a second CFGR1 write can hang the part).
 *       The caller (stm32n6_clockconfig()) checks CPUSWS/SYSSWS
 *       before invoking this path.
 *
 * Returned Value:
 *   0 on success; -1 if PLL1RDY does not assert within the timeout.
 *
 ****************************************************************************/

static inline int rcc_configpll1(void)
{
  uint32_t regval;
  volatile int timeout;

  /* Turn PLL1 off (if running) via the atomic clear alias before
   * reconfiguring its dividers.
   */

  putreg32(RCC_CR_PLL1ON, STM32_RCC_CCR);

  for (timeout = CLOCK_READY_TIMEOUT; timeout > 0; timeout--)
    {
      if ((getreg32(STM32_RCC_SR) & RCC_SR_PLL1RDY) == 0)
        {
          break;
        }
    }

  /* Configure PLL1 source, input divider (M), and feedback divider
   * (N) in a single PLL1CFGR1 write.
   *
   * DIVM/DIVN are direct division/multiplication factors, not
   * "value-1" encodings: confirmed against ST's own HAL driver
   * (stm32n6xx_hal_rcc.c HAL_RCC_OscConfig(): MODIFY_REG(...,
   * pPLLInit->PLLM << RCC_PLL1CFGR1_PLL1DIVM_Pos ...) writes PLLM
   * unmodified) and its header doc ("PLLM ... must be a number
   * between Min_Data = 1 and Max_Data = 63"). A previous revision
   * of this function wrote (STM32_PLL1_M - 1)/(STM32_PLL1_N - 1),
   * which does not match board.h's "M=2 * N=25 = 800MHz" comment
   * and would have programmed the wrong VCO frequency.
   */

#ifdef CONFIG_STM32N6_USE_HSE
  regval = RCC_PLL1CFGR1_SEL_HSE;
#else
  regval = RCC_PLL1CFGR1_SEL_HSI;
#endif

  regval |= (STM32_PLL1_M << RCC_PLL1CFGR1_DIVM_SHIFT) |
            (STM32_PLL1_N << RCC_PLL1CFGR1_DIVN_SHIFT);
  putreg32(regval, STM32_RCC_PLL1CFGR1);

  /* Post-dividers: enable direct VCO output (PDIV1=PDIV2=1) and
   * disable spread-spectrum modulation.
   */

  regval = RCC_PLL1CFGR3_MODSSDIS | RCC_PLL1CFGR3_PDIVEN |
           (1 << RCC_PLL1CFGR3_PDIV1_SHIFT) |
           (1 << RCC_PLL1CFGR3_PDIV2_SHIFT);
  putreg32(regval, STM32_RCC_PLL1CFGR3);

  /* Enable PLL1 via the atomic set alias and wait for ready */

  putreg32(RCC_CR_PLL1ON, STM32_RCC_CSR);

  for (timeout = CLOCK_READY_TIMEOUT; timeout > 0; timeout--)
    {
      if ((getreg32(STM32_RCC_SR) & RCC_SR_PLL1RDY) != 0)
        {
          break;
        }
    }

  if ((getreg32(STM32_RCC_SR) & RCC_SR_PLL1RDY) == 0)
    {
      return -1;
    }

  /* IC dividers: register field is (divider - 1).  IC1 feeds CPUCLK
   * directly; IC2/IC6/IC11 feed the SYSCLK domain switch group; IC3
   * feeds the XSPI2 kernel clock.  The ratios below mirror the
   * upstream STM32N6 port: IC2 = IC1_DIV*2, IC3 = IC1_DIV*4,
   * IC6 = IC1_DIV*3, IC11 = IC1_DIV*2 relative to the VCO.
   */

  putreg32(RCC_ICCFGR_SEL_PLL1 |
           ((STM32_PLL1_IC1_DIV - 1) << RCC_ICCFGR_INT_SHIFT),
           STM32_RCC_IC1CFGR);
  putreg32(RCC_ICCFGR_SEL_PLL1 |
           ((STM32_PLL1_IC1_DIV * 2 - 1) << RCC_ICCFGR_INT_SHIFT),
           STM32_RCC_IC2CFGR);
  putreg32(RCC_ICCFGR_SEL_PLL1 |
           ((STM32_PLL1_IC1_DIV * 4 - 1) << RCC_ICCFGR_INT_SHIFT),
           STM32_RCC_IC3CFGR);
  putreg32(RCC_ICCFGR_SEL_PLL1 |
           ((STM32_PLL1_IC1_DIV * 3 - 1) << RCC_ICCFGR_INT_SHIFT),
           STM32_RCC_IC6CFGR);
  putreg32(RCC_ICCFGR_SEL_PLL1 |
           ((STM32_PLL1_IC1_DIV * 2 - 1) << RCC_ICCFGR_INT_SHIFT),
           STM32_RCC_IC11CFGR);

  putreg32(RCC_DIVENR_IC1EN | RCC_DIVENR_IC2EN | RCC_DIVENR_IC3EN |
           RCC_DIVENR_IC6EN | RCC_DIVENR_IC11EN,
           STM32_RCC_DIVENSR);

  return 0;
}

/****************************************************************************
 * Name: rcc_switchsysclk
 *
 * Description:
 *   Switch CPUCLK/SYSCLK from HSI to the IC1/IC2+IC6+IC11 group
 *   configured by rcc_configpll1().  CFGR2 (bus prescalers) must be
 *   written before CFGR1, and CFGR1 must be written exactly once
 *   with both CPUSW and SYSSW set together -- see the CFGR1 hardware
 *   note in hardware/stm32_rcc.h.
 *
 ****************************************************************************/

static inline void rcc_switchsysclk(void)
{
  uint32_t regval;

  /* AHB prescaler: SYSCLK / 2 (conservative default matching the
   * upstream STM32N6 port; adjust via board.h if a different HCLK
   * ratio is required).
   */

  putreg32(RCC_CFGR2_HPRE_SYSCLKd2, STM32_RCC_CFGR2);

  regval  = getreg32(STM32_RCC_CFGR1);
  regval &= ~(RCC_CFGR1_CPUSW_MASK | RCC_CFGR1_SYSSW_MASK);
  regval |= RCC_CFGR1_CPUSW_IC1 | RCC_CFGR1_SYSSW_IC2_IC6_IC11;
  putreg32(regval, STM32_RCC_CFGR1);

  /* Some SRAM bank clocks can drop out across the clock-domain
   * switch; re-arm them so the heap stays alive.
   */

  putreg32(RCC_MEMENR_ALLAXISRAM | RCC_MEMENR_CACHEAXIRAMEN,
           STM32_RCC_MEMENSR);
}

#endif /* CONFIG_STM32N6_USE_PLL1 */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_clockconfig
 *
 * Description:
 *   Configure system clock. If PLL1 is enabled via Kconfig, this sets
 *   up HSI (or HSE) -> PLL1 -> IC1/IC2+IC6+IC11 per board.h.
 *   Otherwise falls back to HSI 64MHz.
 *
 *   CFGR1 latches after its first post-reset write (see hardware/
 *   stm32_rcc.h); if a prior boot stage (FSBL) already switched
 *   CPUSW/SYSSW to the IC group, skip PLL1/CFGR1 reconfiguration
 *   entirely to avoid a second write that can hang the part.
 *
 ****************************************************************************/

void stm32n6_clockconfig(void)
{
  uint32_t regval;

  /* Enable all AXISRAM bank clocks unconditionally, matching
   * apache/nuttx upstream stm32_rcc_enableperipherals(): the boot
   * ROM only enables AXISRAM1/2, which is sufficient to reach this
   * point, but the NuttX heap extends across all SRAM banks (up to
   * AXISRAM5/6 depending on board.h's memory layout).  Without
   * this, mm_initialize() writing the heap's tail node can fault
   * with an IMPRECISERR bus fault the first time it touches an
   * un-clocked bank.  A previous revision of this function only
   * did this inside rcc_switchsysclk() (i.e. only when
   * CONFIG_STM32N6_USE_PLL1 is enabled and PLL1 configuration
   * succeeds), leaving the default HSI-only build path exposed to
   * the same fault upstream's comment warns about.
   */

  putreg32(RCC_MEMENR_ALLAXISRAM | RCC_MEMENR_CACHEAXIRAMEN,
           STM32_RCC_MEMENSR);

  /* The heap tail sentinel lands in AXISRAM6 when CONFIG_RAM_END is raised
   * past 0x34200000, so the NPU RAM banks (AXISRAM3..6) must be both
   * released from reset and reachable before up_allocate_heap() runs:
   *
   *   - RCC_MEMENR resets to 0x000013F0, i.e. AXISRAM3..6EN (bits 0..3)
   *     are CLEAR: the RM states the CPU must enable these memories before
   *     using them.
   *   - ck_icn_npu (RCC_BUSENR.ACLKNEN) gates the NPU interconnect that
   *     AXISRAM3..6 sit behind.  Per RM0486 14.4, with that clock off the
   *     CPU cannot access AXISRAM3/4/5/6 at all -- writes are dropped
   *     silently and reads come back as zero, which is exactly what the
   *     2026-09-11 probe measured.
   */

  putreg32(0x0000000f, STM32_RCC_MEMRSTCR);   /* release AXISRAM3..6    */
  putreg32(RCC_BUSENR_ACLKNEN | RCC_BUSENR_ACLKNCEN,
           STM32_RCC_BUSENSR);                /* ck_icn_npu / ck_icn_npuc */

  /* Keep the NPU RAM clocks running through CSLEEP (WFI) as well, matching
   * the MEMLPENR handling of the CPU's own banks.
   */

  putreg32(RCC_MEMENR_AXISRAM3EN | RCC_MEMENR_AXISRAM4EN |
           RCC_MEMENR_AXISRAM5EN | RCC_MEMENR_AXISRAM6EN |
           RCC_MEMENR_CACHEAXIRAMEN, STM32_RCC_MEMLPENSR);

  /* Power AXISRAM3..6 on.  RM0486 10.3: "When a RAM is in shutdown, writing
   * in it has no effect, and reading it returns zero" -- measured exactly
   * that on this board (all RCC gates already open, RISAF4/5/6 GLOCK=0).
   * ST's own AI examples do the same with HAL_RAMCFG_EnableAXISRAM() for
   * RAMCFG_SRAM3_AXI..RAMCFG_SRAM6_AXI before using 0x34200000.  Sequence
   * from RM0486 10.3: clear SRAMSD, read back, then the RAM clock (done
   * above) is enough.
   */

  putreg32(RCC_AHB2ENR_RAMCFGEN, STM32_RCC_AHB2ENSR);

  {
    static const uint32_t ramcfg[4] =
    {
      STM32_RAMCFG_SRAM3_AXI, STM32_RAMCFG_SRAM4_AXI,
      STM32_RAMCFG_SRAM5_AXI, STM32_RAMCFG_SRAM6_AXI
    };
    int i;

    for (i = 0; i < 4; i++)
      {
        putreg32(getreg32(ramcfg[i]) & ~RAMCFG_CR_SRAMSD, ramcfg[i]);
        (void)getreg32(ramcfg[i]);   /* read back, RM0486 10.3 step b */
      }
  }

#ifdef CONFIG_STM32N6_USE_PLL1
  regval = getreg32(STM32_RCC_CFGR1);
  if ((regval & RCC_CFGR1_CPUSWS_MASK) == RCC_CFGR1_CPUSWS_IC1 &&
      (regval & RCC_CFGR1_SYSSWS_MASK) == RCC_CFGR1_SYSSWS_IC2_IC6_IC11)
    {
      goto enable_peripherals;
    }
#endif

  /* Always start with HSI as fallback */

  rcc_enablehsi();

#ifdef CONFIG_STM32N6_USE_HSE
  /* Enable HSE and wait for ready */

  if (rcc_enablehse() < 0)
    {
      /* HSE failed, stay on HSI */

      goto enable_peripherals;
    }
#endif

#ifdef CONFIG_STM32N6_USE_PLL1
  /* Configure and enable PLL1, switch system clock */

  if (rcc_configpll1() < 0)
    {
      /* PLL1 failed, stay on HSI */

      goto enable_peripherals;
    }

  rcc_switchsysclk();
#endif

#if defined(CONFIG_STM32N6_USE_HSE) || defined(CONFIG_STM32N6_USE_PLL1)
enable_peripherals:
#endif

  /* Enable GPIOE clock (for USART1 pins PE5/PE6).
   * STM32N6 RCC: the *ENR registers are READ-ONLY status; clock gating
   * is done through the *ENSR (write-1-to-set) aliases.  Writing ENR
   * silently does nothing, which left the console pins unclocked under
   * DEV boot (OpenOCD/GDB load) -> zero UART output.  The FSBL masked
   * this on flash boot because it had already enabled GPIOE/USART1
   * before handing off. */

  putreg32(RCC_AHB4ENR_GPIOEEN, STM32_RCC_AHB4ENSR);

  /* Enable USART1 clock */

  putreg32(RCC_APB2ENR_USART1EN, STM32_RCC_APB2ENSR);
}

/****************************************************************************
 * Name: stm32n6_get_sysclk
 *
 * Description:
 *   Return the current SYSCLK frequency in Hz.
 *
 ****************************************************************************/

uint32_t stm32n6_get_sysclk(void)
{
#ifdef CONFIG_STM32N6_USE_PLL1
#  ifdef STM32_SYSCLK_FREQUENCY
  return STM32_SYSCLK_FREQUENCY;
#  else
  return STM32_HSI_FREQUENCY;
#  endif
#else
  return STM32_HSI_FREQUENCY;
#endif
}

/****************************************************************************
 * Name: stm32n6_get_hclk
 *
 * Description:
 *   Return the AHB bus (HCLK) frequency in Hz.
 *
 ****************************************************************************/

uint32_t stm32n6_get_hclk(void)
{
#if defined(CONFIG_STM32N6_USE_PLL1) && defined(STM32_HCLK_FREQUENCY)
  return STM32_HCLK_FREQUENCY;
#else
  return stm32n6_get_sysclk();
#endif
}

/****************************************************************************
 * Name: stm32n6_get_pclk1
 *
 * Description:
 *   Return the APB1 bus frequency in Hz.
 *
 ****************************************************************************/

uint32_t stm32n6_get_pclk1(void)
{
#if defined(CONFIG_STM32N6_USE_PLL1) && defined(STM32_PCLK1_FREQUENCY)
  return STM32_PCLK1_FREQUENCY;
#else
  return stm32n6_get_hclk();
#endif
}

/****************************************************************************
 * Name: stm32n6_get_pclk2
 *
 * Description:
 *   Return the APB2 bus frequency in Hz.
 *
 ****************************************************************************/

uint32_t stm32n6_get_pclk2(void)
{
#if defined(CONFIG_STM32N6_USE_PLL1) && defined(STM32_PCLK2_FREQUENCY)
  return STM32_PCLK2_FREQUENCY;
#else
  return stm32n6_get_hclk();
#endif
}
