/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_fsbl_regress.c
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

/* FSBL regression probe (2026-08-12).
 *
 * The custom FSBL (contest2026_294_shiqian/firmware) was changed to:
 *   - open RISAF2/RISAF3 (CPU AXI RAM0/1 = AXISRAM1/2) to every CID so the
 *     LTDC (CID0) can read the 800x480 RGB565 framebuffer that spans
 *     FLEXMEM + AXISRAM1 + AXISRAM2;
 *   - keep the LTDC module clock and the SRAM clocks alive during CPU Sleep
 *     (RCC_APB5LPENR.LTDCLPEN + RCC_MEMLPENR FLEXRAM/AXISRAM1/2).
 *
 * Because RISAF rules are global to every bus master, this probe re-checks
 * on real silicon that the widened windows did not break the other CID0
 * masters that share SRAM: DMA2D and GPDMA1 (the NPU master attributes are
 * dumped as read-only configuration evidence; NuttX has no NPU driver).
 *
 * A single real-HW run yields a full pass/fail report:
 *   1. Read-back dump of the RIF / LPENR configuration the FSBL programmed:
 *      RIMC_ATTR[DMA2D/NPU/LTDC1], RISAF2/3/7 REG0 and the intentionally
 *      disabled RISAF4/5/6 REG0, GPDMA1 channel-0 CCIDCFGR, and the RCC
 *      APB5LPENR / MEMLPENR status registers.
 *   2. RIMC_ATTR[DMA2D] writability check (a 0 write must stick), so the
 *      CID sweep in step 3 is meaningful.
 *   3. DMA2D register-to-memory fill sweeping every CID 0..7 (secure+priv)
 *      into the static SRAM buffer, then -- with the first working CID --
 *      into the heap buffer offsets that land in AXISRAM1/AXISRAM2.
 *   4. Explicit REG1 experiment: because every CID write was dropped on
 *      2026-08-12 despite REG0 reading back fully open, program explicit
 *      RISAF2/3 REG1 windows and re-run the DMA2D write to test whether
 *      writes require an explicit STARTR/ENDR region rather than REG0.
 *   5. GPDMA1 memory-to-memory copy through the existing stm32n6_dma API,
 *      source in FLEXMEM, destination in the heap (AXISRAM), verifying the
 *      copied data is byte-correct.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <errno.h>
#include <syslog.h>

#include <nuttx/arch.h>
#include <nuttx/cache.h>
#include <nuttx/kmalloc.h>

#include "arm_internal.h"
#include "stm32n6_dma.h"
#include "hardware/stm32_dma2d.h"
#include "hardware/stm32_memorymap.h"
#include "hardware/stm32_rcc.h"

#ifdef CONFIG_STM32_FSBL_REGRESS

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Cortex-M55 D-cache line size (ADR-007 keeps D-cache enabled). */

#define REGRESS_DCACHE_LINE    32

/* Distinctive markers: the sentinel is what the CPU writes first; the fill
 * value is what DMA2D should overwrite it with.  Neither is 0, so a dropped
 * write (leaving the sentinel) and a zeroed region are both distinguishable
 * from success.
 */

#define REGRESS_SENTINEL       0x11111111u
#define REGRESS_FILL           0x5a5aa55au

/* Bounded spin for the DMA2D flag poll. */

#define REGRESS_POLL_LIMIT     1000000u

/* R2M probe geometry: 8 ARGB8888 pixels == 32 bytes == one whole D-cache
 * line, so clean/invalidate touches only the probed buffer.
 */

#define REGRESS_PROBE_WORDS    8

/* Heap buffer for the AXISRAM probes: 1 MiB + 64 KiB so offsets 0, +512 KiB
 * and +1 MiB are all inside the allocation and span FLEXMEM / AXISRAM1 /
 * AXISRAM2 whatever the heap start address happens to be.
 */

#define REGRESS_BIG_SIZE       0x110000

/* RISAF bases (RM0486 Table 24; the NuttX memory map only exposes RISAF7).
 * Base offset from AHB3: RISAF1 +0x6000 ... RISAF7 +0xc000.
 */

#define STM32_RISAF1_BASE      (STM32_AHB3_BASE + 0x6000)
#define STM32_RISAF2_BASE      (STM32_AHB3_BASE + 0x7000)
#define STM32_RISAF3_BASE      (STM32_AHB3_BASE + 0x8000)
#define STM32_RISAF4_BASE      (STM32_AHB3_BASE + 0x9000)
#define STM32_RISAF5_BASE      (STM32_AHB3_BASE + 0xa000)
#define STM32_RISAF6_BASE      (STM32_AHB3_BASE + 0xb000)

/* RCC MEMLPENR status register (RM0486 14.10.93); the SET alias the FSBL
 * writes is at +0x800 of this offset.
 */

#define STM32_RCC_MEMLPENR     (STM32_RCC_BASE + 0x28c)

/* RIF master indices (ST HAL stm32n6xx_hal_rif.h; stm32_dma2d.h only
 * defines GPU2D=7 and DMA2D=8).
 */

#define REGRESS_MASTER_NPU     1
#define REGRESS_MASTER_LTDC1   10

/* GPDMA1 channel-0 CCIDCFGR: channel 0 register block is at +0x50 past the
 * GPDMA base (per CMSIS), CCIDCFGR is +0x04 inside the channel block.
 */

#define REGRESS_GPDMA1_CH0_CCIDCFGR  (STM32_GPDMA1_BASE + 0x54)

/* GPDMA1 CCIDCFGR bits (CMSIS DMA_CCIDCFGR): CFEN enables CID filtering,
 * SCID selects the static CID.  Static CID0 == SCID field 0.
 */

#define REGRESS_GPDMA_CCIDCFGR_CFEN  (1 << 0)

/* GPDMA1 global security registers (CMSIS DMA_TypeDef): SECCFGR @ +0x00 and
 * PRIVCFGR @ +0x04, one bit per channel.  The 2026-08-12 illegal-access
 * dump shows GPDMA1 presenting NS UNPRIV; setting these makes it SEC+PRIV.
 */

#define REGRESS_GPDMA_SECCFGR   (STM32_GPDMA1_BASE + 0x00)
#define REGRESS_GPDMA_PRIVCFGR  (STM32_GPDMA1_BASE + 0x04)

/* FLEXRAM write target: the LTDC framebuffer in board_bringup.c
 * (g_ltdc_fb, 800x480 RGB565), which the linker placed at 0x3405b440 on
 * 2026-08-12 -- inside FLEXRAM (RISAF7).  Writing 32 bytes here only tints
 * a few pixels and fbcolor can restore them.
 */

#define REGRESS_FB_FLEXMEM    0x3405b440u

/* RISAF illegal-access registers (CMSIS offsets). */

#define REGRESS_RISAF_IASR    0x08   /* Illegal access status */
#define REGRESS_RISAF_IACR    0x0c   /* Illegal access clear (w1c) */
#define REGRESS_RISAF_IAESR   0x20   /* IAR[0] error status detail */
#define REGRESS_RISAF_IADDR   0x24   /* IAR[0] illegal address */

/* SRAM region classification (RM0486 memory map). */

#define REGRESS_AXISRAM1_BASE  0x34080000u
#define REGRESS_AXISRAM2_BASE  0x34180000u
#define REGRESS_AXISRAM3_BASE  0x34280000u

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: regress_sram_region
 *
 * Description:
 *   Classify an SRAM address into the RISAF-protected region it falls in,
 *   for the log lines.
 *
 ****************************************************************************/

static const char *regress_sram_region(uint32_t addr)
{
  if (addr < REGRESS_AXISRAM1_BASE)
    {
      return "FLEXMEM(RISAF7)";
    }
  else if (addr < REGRESS_AXISRAM2_BASE)
    {
      return "AXISRAM1(RISAF2)";
    }
  else if (addr < REGRESS_AXISRAM3_BASE)
    {
      return "AXISRAM2(RISAF3)";
    }

  return "AXISRAM3+";
}

/****************************************************************************
 * Name: regress_risaf_reg0
 *
 * Description:
 *   Dump region 0 of one RISAF container.
 *
 ****************************************************************************/

static void regress_risaf_reg0(const char *name, uint32_t base)
{
  uint32_t reg0 = STM32_RISAF_REG(base, 0);

  syslog(LOG_INFO,
         "regress: %-6s REG0 CFGR=0x%08lx STARTR=0x%08lx "
         "ENDR=0x%08lx CIDCFGR=0x%08lx\n",
         name,
         (unsigned long)getreg32(reg0 + STM32_RISAF_REG_CFGR),
         (unsigned long)getreg32(reg0 + STM32_RISAF_REG_STARTR),
         (unsigned long)getreg32(reg0 + STM32_RISAF_REG_ENDR),
         (unsigned long)getreg32(reg0 + STM32_RISAF_REG_CIDCFGR));
}

/* RISAF instance map for the illegal-access scan. */

static const uint32_t g_regress_risaf_base[7] =
{
  STM32_RISAF1_BASE, STM32_RISAF2_BASE, STM32_RISAF3_BASE,
  STM32_RISAF4_BASE, STM32_RISAF5_BASE, STM32_RISAF6_BASE,
  STM32_RISAF7_BASE
};

static const char *g_regress_risaf_name[7] =
{
  "RISAF1", "RISAF2", "RISAF3", "RISAF4",
  "RISAF5", "RISAF6", "RISAF7"
};

#define REGRESS_NRISAF 7

/****************************************************************************
 * Name: regress_risaf_iar_clear
 *
 * Description:
 *   Clear any pending illegal-access flag on every RISAF so a following
 *   probe is the only source of a new flag.
 *
 ****************************************************************************/

static void regress_risaf_iar_clear(void)
{
  int i;

  for (i = 0; i < REGRESS_NRISAF; i++)
    {
      uint32_t iasr;

      iasr = getreg32(g_regress_risaf_base[i] + REGRESS_RISAF_IASR);
      if (iasr != 0)
        {
          putreg32(iasr, g_regress_risaf_base[i] + REGRESS_RISAF_IACR);
        }
    }
}

/****************************************************************************
 * Name: regress_risaf_iar_check
 *
 * Description:
 *   Dump every RISAF illegal-access flag (which instance, which CID,
 *   secure/priv attributes, read/write, and the illegal address), then
 *   clear the flags.  This pinpoints which RISAF rejected a DMA probe and
 *   with what access attributes -- the decisive evidence for the 2026-08-12
 *   DMA2D/GPDMA write-block finding.
 *
 ****************************************************************************/

static void regress_risaf_iar_check(const char *stage)
{
  int i;

  for (i = 0; i < REGRESS_NRISAF; i++)
    {
      uint32_t iasr;

      iasr = getreg32(g_regress_risaf_base[i] + REGRESS_RISAF_IASR);
      if (iasr != 0)
        {
          uint32_t iaesr;
          uint32_t iaddr;

          iaesr = getreg32(g_regress_risaf_base[i] + REGRESS_RISAF_IAESR);
          iaddr = getreg32(g_regress_risaf_base[i] + REGRESS_RISAF_IADDR);

          syslog(LOG_INFO,
                 "regress: %s %s ILLEGAL ACCESS IASR=0x%08lx "
                 "IAESR=0x%08lx (CID=%lu %s %s %s) IADDR=0x%08lx\n",
                 g_regress_risaf_name[i], stage,
                 (unsigned long)iasr, (unsigned long)iaesr,
                 (unsigned long)(iaesr & 0x7),
                 (iaesr & 0x20) ? "SEC" : "NS",
                 (iaesr & 0x10) ? "PRIV" : "UNPRIV",
                 (iaesr & 0x80) ? "WRITE" : "READ",
                 (unsigned long)iaddr);

          putreg32(iasr, g_regress_risaf_base[i] + REGRESS_RISAF_IACR);
        }
    }
}

/****************************************************************************
 * Name: regress_rif_dump
 *
 * Description:
 *   Read back and log the RIF / LPENR configuration the custom FSBL
 *   programmed.  Purely diagnostic; no side effects.
 *
 ****************************************************************************/

static void regress_rif_dump(void)
{
  uint32_t v;

  syslog(LOG_INFO, "regress: --- RIF / LPENR configuration read-back ---\n");

  v = getreg32(STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));
  syslog(LOG_INFO, "regress: RIMC_ATTR[DMA2D] =0x%08lx "
         "(FSBL expects 0x3 = MCID0|SEC|PRIV)\n", (unsigned long)v);

  v = getreg32(STM32_RIFSC_RIMC_ATTR(REGRESS_MASTER_NPU));
  syslog(LOG_INFO, "regress: RIMC_ATTR[NPU]   =0x%08lx "
         "(FSBL expects 0x3)\n", (unsigned long)v);

  v = getreg32(STM32_RIFSC_RIMC_ATTR(REGRESS_MASTER_LTDC1));
  syslog(LOG_INFO, "regress: RIMC_ATTR[LTDC1] =0x%08lx (info only)\n",
         (unsigned long)v);

  regress_risaf_reg0("RISAF1", STM32_RISAF1_BASE);
  regress_risaf_reg0("RISAF2", STM32_RISAF2_BASE);
  regress_risaf_reg0("RISAF3", STM32_RISAF3_BASE);
  regress_risaf_reg0("RISAF4", STM32_RISAF4_BASE);
  regress_risaf_reg0("RISAF5", STM32_RISAF5_BASE);
  regress_risaf_reg0("RISAF6", STM32_RISAF6_BASE);
  regress_risaf_reg0("RISAF7", STM32_RISAF7_BASE);

  /* The GPDMA1 channel registers (incl. CCIDCFGR) are RAZ/WI while the AHB1
   * peripheral clock is gated, so open the clock before reading -- a 0 then
   * means the FSBL write really never landed, not a clock artifact.  The
   * AHB1 clock gate is set through the AHB1ENSR set-alias (same as the
   * stm32n6_dma driver).
   */

  putreg32(RCC_AHB1ENR_GPDMA1EN, STM32_RCC_AHB1ENSR);

  v = getreg32(REGRESS_GPDMA1_CH0_CCIDCFGR);
  syslog(LOG_INFO, "regress: GPDMA1 ch0 CCIDCFGR =0x%08lx "
         "(FSBL expects 0x1 = static CID0 + CFEN)\n", (unsigned long)v);

  v = getreg32(STM32_RCC_APB5LPENR);
  syslog(LOG_INFO, "regress: RCC APB5LPENR  =0x%08lx "
         "(FSBL expects 0x2 = LTDCLPEN)\n", (unsigned long)v);

  v = getreg32(STM32_RCC_MEMLPENR);
  syslog(LOG_INFO, "regress: RCC MEMLPENR   =0x%08lx "
         "(FSBL expects 0x380 = FLEXRAM|AXISRAM1|AXISRAM2)\n",
         (unsigned long)v);
}

/****************************************************************************
 * Name: regress_dma2d_r2m_fill
 *
 * Description:
 *   Run one DMA2D register-to-memory fill of the known word into an
 *   arbitrary SRAM address and verify it landed.  A sentinel pre-fill
 *   (cleaned to physical SRAM) plus a post-fill D-cache invalidate tells a
 *   real DMA2D write from a dropped (RAZ/WI) firewalled write.
 *
 ****************************************************************************/

static int regress_dma2d_r2m_fill(uint32_t *target, uint32_t words)
{
  uint32_t isr;
  uint32_t count;
  int      i;

  for (i = 0; i < words; i++)
    {
      target[i] = REGRESS_SENTINEL;
    }

  up_clean_dcache((uintptr_t)target,
                  (uintptr_t)target + words * sizeof(uint32_t));

  putreg32(DMA2D_IFCR_CTEIF | DMA2D_IFCR_CTCIF | DMA2D_IFCR_CCEIF,
           STM32_DMA2D_IFCR);
  putreg32(DMA2D_OPFCCR_CM_ARGB8888, STM32_DMA2D_OPFCCR);
  putreg32(REGRESS_FILL, STM32_DMA2D_OCOLR);
  putreg32((uint32_t)(uintptr_t)target, STM32_DMA2D_OMAR);
  putreg32(0, STM32_DMA2D_OOR);
  putreg32((words << DMA2D_NLR_PL_SHIFT) | (1 << DMA2D_NLR_NL_SHIFT),
           STM32_DMA2D_NLR);
  putreg32(DMA2D_CR_MODE_R2M | DMA2D_CR_START, STM32_DMA2D_CR);

  count = 0;
  do
    {
      isr = getreg32(STM32_DMA2D_ISR);
      if (++count > REGRESS_POLL_LIMIT)
        {
          return -ETIMEDOUT;
        }
    }
  while ((isr & (DMA2D_ISR_TCIF | DMA2D_ISR_TEIF | DMA2D_ISR_CEIF)) == 0);

  if ((isr & (DMA2D_ISR_TEIF | DMA2D_ISR_CEIF)) != 0)
    {
      return -EIO;
    }

  up_invalidate_dcache((uintptr_t)target,
                       (uintptr_t)target + words * sizeof(uint32_t));

  for (i = 0; i < words; i++)
    {
      if (target[i] != REGRESS_FILL)
        {
          return -EFAULT;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: regress_dma2d_log
 *
 * Description:
 *   Log one DMA2D probe result with its region classification.
 *
 ****************************************************************************/

static void regress_dma2d_log(const char *region, uint32_t addr, int r)
{
  const char *what;

  if (r == OK)
    {
      what = "write landed";
    }
  else if (r == -EFAULT)
    {
      what = "DROPPED (firewalled)";
    }
  else if (r == -ETIMEDOUT)
    {
      what = "timeout";
    }
  else
    {
      what = "xfer error";
    }

  syslog(LOG_INFO, "regress: DMA2D R2M %-17s @0x%08lx -> %s\n",
         region, (unsigned long)addr, what);
}

/****************************************************************************
 * Name: regress_dma2d_test
 *
 * Description:
 *   DMA2D R2M write probe.  Sweeps every compartment ID 0..7 through RIMC
 *   (secure+priv attributes, matching the FSBL configuration) and reports
 *   which CID can land a write in the static SRAM buffer.  This is the
 *   decisive experiment for the 2026-08-12 finding that DMA2D(CID0) writes
 *   to AXISRAM1 were dropped even though RISAF2/3 REG0 are read back as
 *   open-to-every-CID: if only CID1 lands the RISAF windows are still on
 *   the default rule, if a low CID lands the FSBL grant works, if every
 *   CID is dropped DMA2D is isolated somewhere else (RIFSC master rules).
 *   With the first working CID it then probes the heap buffer offsets that
 *   land in AXISRAM1/AXISRAM2 (RISAF2/3).
 *
 ****************************************************************************/

static int regress_dma2d_test(void)
{
  static uint32_t g_regress_flexmem[REGRESS_PROBE_WORDS]
    aligned_data(REGRESS_DCACHE_LINE);
  uint32_t rimc_before;
  uint32_t *big;
  uint32_t *probe;
  int       found = -1;
  int       ret = OK;
  int       r;
  int       cid;

  syslog(LOG_INFO,
         "regress: --- DMA2D R2M write probe (CID sweep 0..7) ---\n");

  modifyreg32(STM32_RCC_AHB5ENR, 0, RCC_AHB5ENR_DMA2DEN);

  rimc_before = getreg32(STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

  for (cid = 0; cid <= 7; cid++)
    {
      putreg32(RIFSC_RIMC_ATTR_MCID(cid) | RIFSC_RIMC_ATTR_MSEC |
               RIFSC_RIMC_ATTR_MPRIV,
               STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

      r = regress_dma2d_r2m_fill(g_regress_flexmem, REGRESS_PROBE_WORDS);
      syslog(LOG_INFO, "regress: DMA2D CID%d %-17s @0x%08lx -> %s\n",
             cid,
             regress_sram_region((uint32_t)(uintptr_t)g_regress_flexmem),
             (unsigned long)(uintptr_t)g_regress_flexmem,
             r == OK       ? "write landed" :
             r == -EFAULT  ? "DROPPED (firewalled)" :
             r == -ETIMEDOUT ? "timeout" : "xfer error");

      if (r == OK)
        {
          found = cid;
          break;
        }
    }

  putreg32(rimc_before, STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

  if (found < 0)
    {
      /* Every CID was dropped on AXISRAM1.  Distinguish "DMA2D writes are
       * blocked everywhere" from "AXISRAM1/2 specifically rejects writes"
       * by probing FLEXRAM (RISAF7) using the framebuffer, which CPU and
       * LTDC both access freely.
       */

      putreg32(RIFSC_RIMC_ATTR_MCID(0) | RIFSC_RIMC_ATTR_MSEC |
               RIFSC_RIMC_ATTR_MPRIV,
               STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

      r = regress_dma2d_r2m_fill((uint32_t *)REGRESS_FB_FLEXMEM,
                                 REGRESS_PROBE_WORDS);
      syslog(LOG_INFO, "regress: DMA2D CID0 FLEXMEM(RISAF7) fb "
             "@0x%08lx -> %s\n",
             (unsigned long)REGRESS_FB_FLEXMEM,
             r == OK       ? "write landed" :
             r == -EFAULT  ? "DROPPED (firewalled)" :
             r == -ETIMEDOUT ? "timeout" : "xfer error");

      putreg32(rimc_before,
               STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

      syslog(LOG_ERR, "regress: DMA2D: every CID 0..7 was firewalled\n");
      return -EFAULT;
    }

  syslog(LOG_INFO, "regress: DMA2D: CID %d lands writes; probing AXISRAM\n",
         found);

  /* Heap buffer spanning FLEXMEM / AXISRAM1 / AXISRAM2: probe three
   * offsets 512 KiB apart and report the region each absolute address
   * falls into.  kmm_memalign(32) keeps the probes cache-line aligned so
   * clean/invalidate never touches neighbouring heap metadata.
   */

  big = kmm_memalign(REGRESS_DCACHE_LINE, REGRESS_BIG_SIZE);
  if (big == NULL)
    {
      syslog(LOG_ERR, "regress: kmm_memalign(32, 0x%x) failed\n",
             REGRESS_BIG_SIZE);
      return OK;
    }

  putreg32(RIFSC_RIMC_ATTR_MCID(found) | RIFSC_RIMC_ATTR_MSEC |
           RIFSC_RIMC_ATTR_MPRIV,
           STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

  probe = big;
  r = regress_dma2d_r2m_fill(probe, REGRESS_PROBE_WORDS);
  regress_dma2d_log(regress_sram_region((uint32_t)(uintptr_t)probe),
                    (uint32_t)(uintptr_t)probe, r);
  if (r != OK)
    {
      ret = r;
    }

  probe = (uint32_t *)((uintptr_t)big + 0x80000);
  r = regress_dma2d_r2m_fill(probe, REGRESS_PROBE_WORDS);
  regress_dma2d_log(regress_sram_region((uint32_t)(uintptr_t)probe),
                    (uint32_t)(uintptr_t)probe, r);
  if (r != OK)
    {
      ret = r;
    }

  probe = (uint32_t *)((uintptr_t)big + 0x100000);
  r = regress_dma2d_r2m_fill(probe, REGRESS_PROBE_WORDS);
  regress_dma2d_log(regress_sram_region((uint32_t)(uintptr_t)probe),
                    (uint32_t)(uintptr_t)probe, r);
  if (r != OK)
    {
      ret = r;
    }

  kmm_free(big);
  putreg32(rimc_before, STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));
  return ret;
}

/****************************************************************************
 * Name: regress_gpdma_test
 *
 * Description:
 *   GPDMA1 memory-to-memory copy through the existing stm32n6_dma driver
 *   API: source in FLEXMEM, destination in the heap (AXISRAM).  Verifies
 *   the copied data is byte-correct after a D-cache invalidate.
 *
 ****************************************************************************/

static int regress_gpdma_test(void)
{
  static const uint32_t g_regress_src[16] =
  {
    0x11111111u, 0x22222222u, 0x33333333u, 0x44444444u,
    0x55555555u, 0x66666666u, 0x77777777u, 0x88888888u,
    0x99999999u, 0xaaaaaaaa, 0xbbbbbbbb, 0xcccccccc,
    0xdddddddd, 0xeeeeeeee, 0xffffffff, 0x00000000u
  };

  uint32_t *dst;
  int       ret = OK;
  int       r;
  int       i;

  syslog(LOG_INFO, "regress: --- GPDMA1 M2M copy probe (CID0) ---\n");

  /* The GPDMA1 channel registers (incl. CCIDCFGR) are RAZ/WI while the AHB1
   * peripheral clock is gated.  Open the clock first, then make sure the
   * channel presents static CID0 with filtering enabled -- the FSBL's write
   * of CCIDCFGR was ignored on 2026-08-12 (read back 0), most likely because
   * it ran before the GPDMA1 clock was enabled, so re-apply it here to make
   * the test deterministic.
   */

  modifyreg32(STM32_RCC_AHB5ENR, 0, RCC_AHB5ENR_DMA2DEN);
  putreg32(RCC_AHB1ENR_GPDMA1EN, STM32_RCC_AHB1ENSR);

  /* The 2026-08-12 illegal-access dump showed GPDMA1 presenting NS UNPRIV
   * CID1 (the FSBL CCIDCFGR write was ignored because the GPDMA1 clock was
   * gated).  Make the controller SEC+PRIV on every channel via SECCFGR /
   * PRIVCFGR and pin channel 0 to static CID0 -- this tests whether a
   * SEC+PRIV GPDMA can reach the secure RISAF windows (path B).
   */

  putreg32(0xffu, REGRESS_GPDMA_SECCFGR);
  putreg32(0xffu, REGRESS_GPDMA_PRIVCFGR);
  putreg32(REGRESS_GPDMA_CCIDCFGR_CFEN, REGRESS_GPDMA1_CH0_CCIDCFGR);

  dst = kmm_memalign(REGRESS_DCACHE_LINE, 64 + REGRESS_DCACHE_LINE);
  if (dst == NULL)
    {
      return -ENOMEM;
    }

  /* Push the source pattern to physical SRAM and drop stale destination
   * cache lines so the DMA write and the CPU read-back see real memory.
   * The destination is 32-byte aligned (kmm_memalign) so the invalidate
   * range never touches neighbouring heap metadata.
   */

  up_clean_dcache((uintptr_t)g_regress_src,
                  (uintptr_t)g_regress_src + sizeof(g_regress_src));
  up_invalidate_dcache((uintptr_t)dst, (uintptr_t)dst + 64);

  r = stm32n6_dma_init(0);
  if (r != OK)
    {
      ret = r;
      goto out;
    }

  r = stm32n6_dma_start(0, (uint32_t)(uintptr_t)g_regress_src,
                        (uint32_t)(uintptr_t)dst, 64);
  if (r != OK)
    {
      ret = r;
      goto out;
    }

  r = stm32n6_dma_wait(0, 1000);
  if (r != OK)
    {
      ret = r;
      goto out;
    }

  up_invalidate_dcache((uintptr_t)dst, (uintptr_t)dst + 64);

  for (i = 0; i < 16; i++)
    {
      if (dst[i] != g_regress_src[i])
        {
          ret = -EFAULT;
          break;
        }
    }

  syslog(LOG_INFO, "regress: GPDMA1 M2M %-17s dst @0x%08lx -> %s\n",
         regress_sram_region((uint32_t)(uintptr_t)dst),
         (unsigned long)(uintptr_t)dst,
         ret == OK ? "copy verified" : "COPY MISMATCH");

out:
  stm32n6_dma_deinit(0);
  kmm_free(dst);
  return ret;
}

/****************************************************************************
 * Name: regress_rimc_writable
 *
 * Description:
 *   Verify that RIMC_ATTR[DMA2D] is actually writable.  The 2026-08-12 CID
 *   sweep dropped every CID 0..7, which is only a meaningful result if the
 *   RIMC write really changed the presented CID.  Write 0 and read it back:
 *   if it sticks, the sweep was real; if it reads back 0x300 the register
 *   is read-only (or locked) and every sweep iteration presented CID0.
 *
 ****************************************************************************/

static int regress_rimc_writable(void)
{
  uint32_t before;
  uint32_t after;
  int      ret = OK;

  syslog(LOG_INFO, "regress: --- RIMC_ATTR[DMA2D] writability ---\n");

  before = getreg32(STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

  putreg32(0, STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));
  after = getreg32(STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

  putreg32(before, STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

  if (after != 0)
    {
      ret = -EIO;
    }

  syslog(LOG_INFO, "regress: RIMC_ATTR[DMA2D] before=0x%08lx "
         "after=0x%08lx -> %s\n",
         (unsigned long)before, (unsigned long)after,
         ret == OK ? "writable" : "WRITE IGNORED");
  return ret;
}

/****************************************************************************
 * Name: regress_risaf_open_region
 *
 * Description:
 *   Open one explicit RISAF region to every CID (read+write, secure,
 *   privileged).  Disable first (BREN=0), program, enable last -- the
 *   ordering the FSBL macro uses and that RM0486 requires.
 *
 ****************************************************************************/

static void regress_risaf_open_region(uint32_t base, int region,
                                      uint32_t endr)
{
  uint32_t reg = STM32_RISAF_REG(base, region);

  putreg32(0, reg + STM32_RISAF_REG_CFGR);
  putreg32(0, reg + STM32_RISAF_REG_STARTR);
  putreg32(endr, reg + STM32_RISAF_REG_ENDR);
  putreg32(0x00ff00ffu, reg + STM32_RISAF_REG_CIDCFGR);
  putreg32(0x00ff0101u, reg + STM32_RISAF_REG_CFGR);
}

/****************************************************************************
 * Name: regress_risaf_open_ns_region
 *
 * Description:
 *   Open one explicit RISAF region to every CID with SEC=0 (a non-secure
 *   region).  The 2026-08-12 illegal-access dump proved DMA2D/GPDMA present
 *   NS transactions regardless of RIMC MSEC, so a secure (SEC=1) region can
 *   never admit them; a non-secure region may.  This is the experiment that
 *   decides whether the fix is "make the regions non-secure" (path A).
 *
 ****************************************************************************/

static void regress_risaf_open_ns_region(uint32_t base, int region,
                                         uint32_t endr)
{
  uint32_t reg = STM32_RISAF_REG(base, region);

  putreg32(0, reg + STM32_RISAF_REG_CFGR);
  putreg32(0, reg + STM32_RISAF_REG_STARTR);
  putreg32(endr, reg + STM32_RISAF_REG_ENDR);
  putreg32(0x00ff00ffu, reg + STM32_RISAF_REG_CIDCFGR);
  putreg32(0x00ff0001u, reg + STM32_RISAF_REG_CFGR);  /* BREN|PRIVC, SEC=0 */
}

/****************************************************************************
 * Name: regress_region1_test
 *
 * Description:
 *   Decisive experiment for the 2026-08-12 finding that DMA2D/GPDMA writes
 *   to AXISRAM1/2 are dropped even though RISAF2/3 REG0 read back fully
 *   open: the illegal-access dump showed the masters present NS
 *   transactions, so a secure region rejects them regardless of CID or
 *   REG0/REG1.  Here REG1 is programmed as a NON-SECURE (SEC=0) window
 *   over AXISRAM1/2 and the DMA2D write is re-run: landed => the fix is to
 *   open non-secure windows (path A); still dropped => the write path is
 *   blocked elsewhere.  The LTDC keeps reading the framebuffer through
 *   REG0 (SEC=1) the whole time, so a still-visible LCD proves a secure
 *   master can also keep working beside the NS window.
 *
 ****************************************************************************/

static int regress_region1_test(void)
{
  static uint32_t g_regress_r1[REGRESS_PROBE_WORDS]
    aligned_data(REGRESS_DCACHE_LINE);
  uint32_t rimc_before;
  int      ret = OK;
  int      r;

  syslog(LOG_INFO,
         "regress: --- RISAF explicit REG1 NS-window experiment ---\n");

  rimc_before = getreg32(STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));
  putreg32(RIFSC_RIMC_ATTR_MCID(0) | RIFSC_RIMC_ATTR_MSEC |
           RIFSC_RIMC_ATTR_MPRIV,
           STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

  /* Baseline with the FSBL configuration (secure REG0 only). */

  r = regress_dma2d_r2m_fill(g_regress_r1, REGRESS_PROBE_WORDS);
  syslog(LOG_INFO, "regress: REG1 baseline CID0 %-17s @0x%08lx -> %s\n",
         regress_sram_region((uint32_t)(uintptr_t)g_regress_r1),
         (unsigned long)(uintptr_t)g_regress_r1,
         r == OK ? "write landed" : "DROPPED (firewalled)");

  /* Open NON-SECURE REG1 windows over AXISRAM1/2 (SEC=0). */

  regress_risaf_open_ns_region(STM32_RISAF2_BASE, 1, 0x000fffff);
  regress_risaf_open_ns_region(STM32_RISAF3_BASE, 1, 0x000fffff);

  r = regress_dma2d_r2m_fill(g_regress_r1, REGRESS_PROBE_WORDS);
  syslog(LOG_INFO, "regress: REG1 NS-opened CID0 %-17s @0x%08lx -> %s\n",
         regress_sram_region((uint32_t)(uintptr_t)g_regress_r1),
         (unsigned long)(uintptr_t)g_regress_r1,
         r == OK ? "write landed" : "DROPPED (firewalled)");
  if (r != OK)
    {
      ret = r;
    }

  syslog(LOG_INFO,
         "regress: NOTE: LCD still visible after NS REG1 => secure LTDC "
         "reads still pass beside the NS window\n");

  /* Restore: disable REG1 (BREN off), REG0 stays as the FSBL left it. */

  putreg32(0, STM32_RISAF_REG(STM32_RISAF2_BASE, 1) + STM32_RISAF_REG_CFGR);
  putreg32(0, STM32_RISAF_REG(STM32_RISAF3_BASE, 1) + STM32_RISAF_REG_CFGR);

  putreg32(rimc_before, STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));
  return ret;
}

/****************************************************************************
 * Name: regress_word_landed
 *
 * Description:
 *   Probe one word: is the address writable by the CPU?  Cache maintenance
 *   brackets the access so a D-Cache hit can never masquerade as a landed
 *   write.  A RIF-denied write is silently discarded and reads back as
 *   zero -- the 2026-09-11 observation that proved the NuttX heap was
 *   extending into memory the CPU cannot write.
 *
 ****************************************************************************/

static int regress_word_landed(uint32_t addr)
{
  FAR volatile uint32_t *p = (FAR volatile uint32_t *)(uintptr_t)addr;
  uint32_t pattern = 0x5a5a0000u | ((addr >> 16) & 0xffffu);

  up_invalidate_dcache((uintptr_t)addr, (uintptr_t)addr + sizeof(uint32_t));
  *p = pattern;
  up_clean_dcache((uintptr_t)addr, (uintptr_t)addr + sizeof(uint32_t));
  up_invalidate_dcache((uintptr_t)addr, (uintptr_t)addr + sizeof(uint32_t));

  return *p == pattern ? OK : -EFAULT;
}

/****************************************************************************
 * Name: regress_ram_window_test
 *
 * Description:
 *   Measure which parts of the 0x34200000-0x34400000 window (SRAM3..SRAM6,
 *   which RM0486 Table 24 calls "the 2-Mbyte NPU RAM", plus CACHEAXI RAM)
 *   the CPU can actually write.  RISAF4/5/6 protect the NPU RAM and reset
 *   to "secure, privileged, CID = 1 only" (RM0486 7.4.3), so the CPU is
 *   expected to be denied until those regions are opened.
 *
 *   Every probed address must be ABOVE the heap end, otherwise the probe
 *   would overwrite live heap bookkeeping.
 *
 ****************************************************************************/

/****************************************************************************
 * Name: regress_npuram_gates_dump
 *
 * Description:
 *   Read back every gate that stands between the CPU and the NPU RAM
 *   (AXISRAM3..6, 0x34200000-0x343C0000):
 *
 *     RCC_MEMENR   reset value 0x000013F0 -> AXISRAM3..6EN (bits 0..3)
 *                  are CLEAR, the CPU must set them (RM0486 14.10.78)
 *     RCC_MEMRSTR  per-bank reset bits (RM0486 14.10.63)
 *     RCC_BUSENR   ACLKNEN gates ck_icn_npu; with it off the CPU cannot
 *                  access AXISRAM3/4/5/6 at all (RM0486 14.4)
 *     SYSCFG_NPU_ICNCR  INTERLEAVING_ACTIVE (RM0486 16.1.21)
 *     RISAF4/5/6 REG0   the RIF view of the NPU RAM
 *
 ****************************************************************************/

#define REGRESS_SYSCFG_NPU_ICNCR (STM32_SYSCFG_BASE + 0x78)

static void regress_npuram_gates_dump(void)
{
  uint32_t memenr = getreg32(STM32_RCC_MEMENR);
  uint32_t rstr   = getreg32(STM32_RCC_MEMRSTR);
  uint32_t busenr = getreg32(STM32_RCC_BUSENR);
  uint32_t icncr  = getreg32(REGRESS_SYSCFG_NPU_ICNCR);

  syslog(LOG_INFO, "regress: --- NPU RAM (AXISRAM3..6) gate state ---\n");
  syslog(LOG_INFO,
         "regress: RCC MEMENR =0x%08lx AXISRAM3..6EN=%d%d%d%d "
         "CACHEAXIRAMEN=%d FLEXRAMEN=%d AXISRAM1/2EN=%d%d\n",
         (unsigned long)memenr,
         (int)((memenr >> 0) & 1u), (int)((memenr >> 1) & 1u),
         (int)((memenr >> 2) & 1u), (int)((memenr >> 3) & 1u),
         (int)((memenr >> 10) & 1u), (int)((memenr >> 9) & 1u),
         (int)((memenr >> 7) & 1u), (int)((memenr >> 8) & 1u));
  syslog(LOG_INFO,
         "regress: RCC MEMRSTR=0x%08lx AXISRAM3..6RST=%d%d%d%d\n",
         (unsigned long)rstr,
         (int)((rstr >> 0) & 1u), (int)((rstr >> 1) & 1u),
         (int)((rstr >> 2) & 1u), (int)((rstr >> 3) & 1u));
  syslog(LOG_INFO,
         "regress: RCC BUSENR =0x%08lx ACLKNEN=%d ACLKNCEN=%d\n",
         (unsigned long)busenr,
         (int)((busenr >> 0) & 1u), (int)((busenr >> 1) & 1u));
  syslog(LOG_INFO,
         "regress: SYSCFG NPU_ICNCR=0x%08lx INTERLEAVING_ACTIVE=%d\n",
         (unsigned long)icncr, (int)(icncr & 1u));
  syslog(LOG_INFO, "regress: RCC AHB2ENR =0x%08lx RAMCFGEN=%d\n",
         (unsigned long)getreg32(STM32_RCC_AHB2ENR),
         (int)((getreg32(STM32_RCC_AHB2ENR) >> 12) & 1u));

  /* RAMCFG shutdown state.  RM0486 10.3: a RAM in shutdown ignores writes
   * and reads back zero -- the exact 2026-09-11 symptom.
   */

  syslog(LOG_INFO,
         "regress: RAMCFG SRAM3/4/5/6 CR=0x%08lx 0x%08lx 0x%08lx 0x%08lx "
         "SRAMSD=%d%d%d%d\n",
         (unsigned long)getreg32(STM32_RAMCFG_SRAM3_AXI),
         (unsigned long)getreg32(STM32_RAMCFG_SRAM4_AXI),
         (unsigned long)getreg32(STM32_RAMCFG_SRAM5_AXI),
         (unsigned long)getreg32(STM32_RAMCFG_SRAM6_AXI),
         (int)((getreg32(STM32_RAMCFG_SRAM3_AXI) >> 20) & 1u),
         (int)((getreg32(STM32_RAMCFG_SRAM4_AXI) >> 20) & 1u),
         (int)((getreg32(STM32_RAMCFG_SRAM5_AXI) >> 20) & 1u),
         (int)((getreg32(STM32_RAMCFG_SRAM6_AXI) >> 20) & 1u));
}

/****************************************************************************
 * Name: regress_npuram_interleave_test
 *
 * Description:
 *   Last candidate gate: SYSCFG_NPU_ICNCR.INTERLEAVING_ACTIVE.  The NPU RAM
 *   is implemented as four interleaved RAM cuts and the HAL exposes
 *   HAL_SYSCFG_EnableInterleavingCpuRam() for exactly this bit, so it is a
 *   supported operation.  Probes with the bit set, then restores it.
 *
 ****************************************************************************/

static int regress_npuram_interleave_test(void)
{
  uint32_t before = getreg32(REGRESS_SYSCFG_NPU_ICNCR);
  int      ret    = OK;

  syslog(LOG_INFO,
         "regress: --- SYSCFG NPU RAM interleaving experiment ---\n");

  putreg32(before | 1u, REGRESS_SYSCFG_NPU_ICNCR);

  syslog(LOG_INFO, "regress: NPU_ICNCR 0x%08lx -> 0x%08lx\n",
         (unsigned long)before,
         (unsigned long)getreg32(REGRESS_SYSCFG_NPU_ICNCR));

  if (regress_word_landed(0x34200000u) == OK)
    {
      syslog(LOG_INFO, "regress: SRAM3 @0x34200000 -> landed\n");
    }
  else
    {
      syslog(LOG_INFO, "regress: SRAM3 @0x34200000 -> still DROPPED\n");
      ret = -1;
    }

  putreg32(before, REGRESS_SYSCFG_NPU_ICNCR);
  syslog(LOG_INFO, "regress: NPU_ICNCR restored to 0x%08lx\n",
         (unsigned long)before);

  return ret;
}

static const uint32_t g_regress_ramprobe[] =
{
  0x34200000u,   /* SRAM3 (NPU RAM)  */
  0x34270000u,   /* SRAM4 (NPU RAM)  */
  0x342e0000u,   /* SRAM5 (NPU RAM)  */
  0x34350000u,   /* SRAM6 (NPU RAM)  */
  0x343c0000u,   /* CACHEAXI RAM base */
  0x343ff000u    /* CACHEAXI RAM top  */
};

static int regress_ram_window_test(void)
{
  uint32_t ahb5 = getreg32(STM32_RCC_AHB5ENR);
  int      ret  = OK;
  unsigned i;

  syslog(LOG_INFO,
         "regress: --- NPU RAM (SRAM3..6) + CACHEAXI writability ---\n");
  syslog(LOG_INFO,
         "regress: RCC AHB5ENR=0x%08lx NPUEN=%d CACHEAXIEN=%d\n",
         (unsigned long)ahb5,
         (int)((ahb5 >> 31) & 1u), (int)((ahb5 >> 30) & 1u));

  for (i = 0; i < sizeof(g_regress_ramprobe) / sizeof(g_regress_ramprobe[0]);
       i++)
    {
      uint32_t addr = g_regress_ramprobe[i];
      int      r    = regress_word_landed(addr);

      syslog(LOG_INFO, "regress: RAM @0x%08lx -> %s\n",
             (unsigned long)addr,
             r == OK ? "landed" : "DROPPED (RIF-denied)");

      if (r != OK)
        {
          ret = r;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: regress_risaf_cr_dump
 *
 * Description:
 *   Read RISAF_CR of every instance.  GLOCK=1 means the boot ROM (or the
 *   secure boot chain) already locked that RISAF configuration until the
 *   next reset (RM0486 7.4.6); the FSBL's HardFault on RISAF4/5/6 writes
 *   would then be unfixable from software.
 *
 ****************************************************************************/

static void regress_risaf_cr_dump(void)
{
  int i;

  syslog(LOG_INFO, "regress: --- RISAF_CR (GLOCK bit) ---\n");

  for (i = 0; i < REGRESS_NRISAF; i++)
    {
      uint32_t cr = getreg32(g_regress_risaf_base[i]);  /* CR is offset 0 */

      syslog(LOG_INFO, "regress: %s CR=0x%08lx GLOCK=%d\n",
             g_regress_risaf_name[i], (unsigned long)cr, (int)(cr & 1u));
    }
}

/****************************************************************************
 * Name: regress_risaf6_open_test
 *
 * Description:
 *   Decisive experiment: program RISAF6 (the CPU master port to the NPU RAM)
 *   exactly the way the FSBL macro programs RISAF2/3/7 -- BREN | SEC | all
 *   CIDs, read+write, privileged -- and re-probe SRAM3/SRAM5.
 *
 *   Landed  => the 2-Mbyte NPU RAM can be handed to the CPU, and the heap
 *              may go back to 0x34400000.
 *   Faulted => the FSBL failure is reproduced with the NPU/CACHEAXI clocks
 *              already running, which rules the clock theory out.
 *
 *   Deliberately runs LAST: a HardFault takes the system down.
 *
 ****************************************************************************/

static int regress_risaf6_open_test(void)
{
  uint32_t reg0        = STM32_RISAF_REG(STM32_RISAF6_BASE, 0);
  uint32_t cfgr_before = getreg32(reg0 + STM32_RISAF_REG_CFGR);
  uint32_t endr_before = getreg32(reg0 + STM32_RISAF_REG_ENDR);
  uint32_t cid_before  = getreg32(reg0 + STM32_RISAF_REG_CIDCFGR);
  int      ret         = OK;

  syslog(LOG_INFO,
         "regress: --- RISAF6 (CPU port to NPU RAM) open experiment ---\n");
  syslog(LOG_INFO, "regress: WARNING: the register write below may "
                   "HardFault\n");

  regress_risaf_open_region(STM32_RISAF6_BASE, 0, 0x3fffffffu);

  syslog(LOG_INFO, "regress: RISAF6 REG0 CFGR 0x%08lx -> 0x%08lx\n",
         (unsigned long)cfgr_before,
         (unsigned long)getreg32(reg0 + STM32_RISAF_REG_CFGR));

  if (regress_word_landed(0x34200000u) == OK)
    {
      syslog(LOG_INFO,
             "regress: SRAM3 @0x34200000 -> landed (CPU owns NPU RAM)\n");
    }
  else
    {
      syslog(LOG_INFO, "regress: SRAM3 @0x34200000 -> still DROPPED\n");
      ret = -1;
    }

  if (regress_word_landed(0x342e0000u) == OK)
    {
      syslog(LOG_INFO, "regress: SRAM5 @0x342e0000 -> landed\n");
    }
  else
    {
      syslog(LOG_INFO, "regress: SRAM5 @0x342e0000 -> still DROPPED\n");
      ret = -1;
    }

  /* Put the region back exactly as it was found (disabled). */

  putreg32(0, reg0 + STM32_RISAF_REG_CFGR);
  putreg32(0, reg0 + STM32_RISAF_REG_STARTR);
  putreg32(endr_before, reg0 + STM32_RISAF_REG_ENDR);
  putreg32(cid_before, reg0 + STM32_RISAF_REG_CIDCFGR);
  putreg32(cfgr_before, reg0 + STM32_RISAF_REG_CFGR);

  syslog(LOG_INFO, "regress: RISAF6 restored to CFGR=0x%08lx\n",
         (unsigned long)cfgr_before);

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_fsbl_regress
 *
 * Description:
 *   Run the FSBL regression probe: RIF/LPENR configuration read-back,
 *   RIMC writability, DMA2D R2M writes into AXISRAM1/2, the explicit
 *   REG1 experiment, and a GPDMA1 M2M copy.  Logs every step; returns OK
 *   only if every probe landed.
 *
 * Returned Value:
 *   OK on success; a negated errno otherwise.
 *
 ****************************************************************************/

int stm32n6_fsbl_regress(void)
{
  int ret = OK;

  syslog(LOG_INFO,
         "regress: ===== FSBL regression: RISAF2/3 + LPENR =====\n");

  regress_rif_dump();
  regress_risaf_cr_dump();
  regress_npuram_gates_dump();

  if (regress_rimc_writable() != OK)
    {
      ret = -1;
    }

  regress_risaf_iar_clear();
  if (regress_dma2d_test() != OK)
    {
      ret = -1;
    }

  regress_risaf_iar_check("after DMA2D sweep");

  regress_risaf_iar_clear();
  if (regress_region1_test() != OK)
    {
      ret = -1;
    }

  regress_risaf_iar_check("after REG1 experiment");

  regress_risaf_iar_clear();
  if (regress_gpdma_test() != OK)
    {
      ret = -1;
    }

  regress_risaf_iar_check("after GPDMA M2M");

  regress_risaf_iar_clear();
  if (regress_ram_window_test() != OK)
    {
      ret = -1;
    }

  regress_risaf_iar_check("after NPU RAM probe");

  regress_risaf_iar_clear();
  if (regress_risaf6_open_test() != OK)
    {
      ret = -1;
    }

  regress_risaf_iar_check("after RISAF6 experiment");

  regress_risaf_iar_clear();
  if (regress_npuram_interleave_test() != OK)
    {
      ret = -1;
    }

  regress_risaf_iar_check("after interleaving experiment");

  syslog(LOG_INFO, "regress: ===== overall: %s =====\n",
         ret == OK ? "PASS" : "FAIL");

  return ret;
}

#endif /* CONFIG_STM32_FSBL_REGRESS */
