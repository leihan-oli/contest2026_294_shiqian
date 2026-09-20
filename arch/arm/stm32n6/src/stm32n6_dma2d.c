/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_dma2d.c
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

/* ADR-032: DMA2D de-risk probe.
 *
 * Before committing to a full DMA2D driver this helper answers one question
 * that only real silicon can settle: in DEV boot, can DMA2D actually land a
 * write in SRAM, or is it firewalled the way GPDMA1 is?  GPDMA1 cannot be
 * unblocked because it is not a RIF-configurable master; DMA2D *is* (master
 * index 8), so its compartment ID (CID) can be reprogrammed via RIFSC.
 *
 * The probe: read the RISAF7 region that covers firmware SRAM to learn which
 * CID it grants write access, present that CID from DMA2D via RIMC, then run
 * a register-to-memory (R2M) fill of a known word into a cache-line-aligned
 * SRAM buffer and read it back.  A sentinel pre-fill (cleaned to physical
 * SRAM) plus a post-fill D-cache invalidate distinguishes a real DMA2D write
 * from a silently dropped (RAZ/WI) firewalled write -- the same technique
 * that exposed the GPDMA1 block.  Everything is logged so a single real-HW
 * run yields the full diagnosis whether it passes or fails.
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

#include "arm_internal.h"
#include "hardware/stm32_dma2d.h"
#include "hardware/stm32_rcc.h"

#ifdef CONFIG_STM32_DMA2D

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Cortex-M55 D-cache line size (ADR-007 keeps D-cache enabled). */

#define DMA2D_DCACHE_LINE   32

/* R2M probe geometry: one line of 8 ARGB8888 pixels == 32 bytes == one
 * whole D-cache line, so the invalidate touches only this buffer.
 */

#define DMA2D_PROBE_PIXELS  8

/* Distinctive markers: the sentinel is what the CPU writes first; the fill
 * value is what DMA2D should overwrite it with.  Neither is 0, so a dropped
 * write (leaving the sentinel) and a zeroed region are both distinguishable
 * from success.
 */

#define DMA2D_PROBE_SENTINEL 0x11111111u
#define DMA2D_PROBE_FILL     0xdeadbeefu

/* Bounded spin: the R2M of 8 pixels completes in a handful of cycles; this
 * keeps a stuck flag from hanging the caller.
 */

#define DMA2D_POLL_LIMIT    1000000

/* RISAF7 covers the FLEXMEM extension where the DEV-boot firmware (and this
 * buffer) live.  Region 0 is the base region for that window.
 */

#define DMA2D_PROBE_RISAF_BASE   STM32_RISAF7_BASE
#define DMA2D_PROBE_RISAF_REGION 0

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* R2M output buffer: one D-cache line, aligned so its clean/invalidate
 * touches only these 32 bytes.
 */

static uint32_t g_dma2d_probe_buf[DMA2D_PROBE_PIXELS]
  aligned_data(DMA2D_DCACHE_LINE);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dma2d_r2m_try
 *
 * Description:
 *   Present the given RIMC master-attribute value to DMA2D, then run one
 *   register-to-memory fill of the known word into the SRAM buffer and
 *   check whether it landed.  A sentinel pre-fill (cleaned to physical
 *   SRAM) plus a post-fill D-cache invalidate tells a real write from a
 *   dropped (RAZ/WI) firewalled write.
 *
 * Returned Value:
 *   OK if the fill word landed in SRAM; -ETIMEDOUT if DMA2D never completed,
 *   -EIO on transfer/config error, -EFAULT if the write was dropped.
 *
 ****************************************************************************/

static int dma2d_r2m_try(uint32_t rimc_attr)
{
  uint32_t isr;
  uint32_t count;
  int      i;

  /* Present the requested compartment ID / attributes for DMA2D. */

  putreg32(rimc_attr, STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));

  /* Pre-fill with the sentinel and push it to physical SRAM so a dropped
   * DMA2D write leaves the sentinel visible after invalidate.
   */

  for (i = 0; i < DMA2D_PROBE_PIXELS; i++)
    {
      g_dma2d_probe_buf[i] = DMA2D_PROBE_SENTINEL;
    }

  up_clean_dcache((uintptr_t)g_dma2d_probe_buf,
                  (uintptr_t)g_dma2d_probe_buf + sizeof(g_dma2d_probe_buf));

  /* Clear any stale DMA2D flags. */

  putreg32(DMA2D_IFCR_CTEIF | DMA2D_IFCR_CTCIF | DMA2D_IFCR_CCEIF,
           STM32_DMA2D_IFCR);

  /* Configure the R2M fill: ARGB8888 output, the known fill colour, the SRAM
   * destination, no inter-line gap, one line of DMA2D_PROBE_PIXELS pixels.
   */

  putreg32(DMA2D_OPFCCR_CM_ARGB8888, STM32_DMA2D_OPFCCR);
  putreg32(DMA2D_PROBE_FILL, STM32_DMA2D_OCOLR);
  putreg32((uint32_t)(uintptr_t)g_dma2d_probe_buf, STM32_DMA2D_OMAR);
  putreg32(0, STM32_DMA2D_OOR);
  putreg32((DMA2D_PROBE_PIXELS << DMA2D_NLR_PL_SHIFT) |
           (1 << DMA2D_NLR_NL_SHIFT), STM32_DMA2D_NLR);

  /* Launch: register-to-memory mode + START. */

  putreg32(DMA2D_CR_MODE_R2M | DMA2D_CR_START, STM32_DMA2D_CR);

  /* Wait for transfer-complete, transfer-error, or config-error. */

  count = 0;
  do
    {
      isr = getreg32(STM32_DMA2D_ISR);
      if (++count > DMA2D_POLL_LIMIT)
        {
          return -ETIMEDOUT;
        }
    }
  while ((isr & (DMA2D_ISR_TCIF | DMA2D_ISR_TEIF | DMA2D_ISR_CEIF)) == 0);

  if ((isr & (DMA2D_ISR_TEIF | DMA2D_ISR_CEIF)) != 0)
    {
      return -EIO;
    }

  /* Invalidate so the read-back comes from physical SRAM, not the cached
   * sentinel, then verify every word was overwritten with the fill value.
   */

  up_invalidate_dcache((uintptr_t)g_dma2d_probe_buf,
                       (uintptr_t)g_dma2d_probe_buf +
                       sizeof(g_dma2d_probe_buf));

  for (i = 0; i < DMA2D_PROBE_PIXELS; i++)
    {
      if (g_dma2d_probe_buf[i] != DMA2D_PROBE_FILL)
        {
          return -EFAULT;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_dma2d_probe
 *
 * Description:
 *   DMA2D register-to-memory write probe.  DMA2D is a RIF-configurable
 *   master (index 8) whose compartment ID (CID) can be reprogrammed via
 *   RIFSC, so a firewalled SRAM write is a CID mismatch rather than the
 *   architectural dead-end that blocks GPDMA1.  A first run with CID 0 was
 *   dropped (RAZ/WI: TCIF set, no error, sentinel intact), so this sweeps
 *   every CID 0..7 with secure+privileged attributes and reports which (if
 *   any) lets the write land.  A single real-HW run therefore settles
 *   whether ADR-032 is viable (some CID works) or PARTIAL (none do).
 *
 * Returned Value:
 *   OK if some CID let DMA2D write SRAM; -EFAULT if every CID was
 *   firewalled; another negated errno on a hardware fault during the sweep.
 *
 ****************************************************************************/

int stm32n6_dma2d_probe(void)
{
  uint32_t rimc_before;
  uint32_t attr;
  int      ret;
  int      cid;

  /* Bring up the RIFSC and DMA2D clocks (atomic read-modify-write). */

  modifyreg32(STM32_RCC_AHB3ENR, 0, RCC_AHB3ENR_RIFSCEN);
  modifyreg32(STM32_RCC_AHB5ENR, 0, RCC_AHB5ENR_DMA2DEN);

  rimc_before = getreg32(STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DMA2D));
  syslog(LOG_INFO, "dma2d: RIMC_ATTR[8] before=0x%08lx, sweeping CID 0..7 "
         "(secure+priv)\n", (unsigned long)rimc_before);

  /* Sweep every compartment ID with secure+privileged attributes (matching a
   * Secure-only boot).  The first CID whose write lands identifies the CID
   * the firmware-SRAM RISAF region trusts.
   */

  for (cid = 0; cid <= 7; cid++)
    {
      attr = RIFSC_RIMC_ATTR_MCID(cid) | RIFSC_RIMC_ATTR_MSEC |
             RIFSC_RIMC_ATTR_MPRIV;
      ret = dma2d_r2m_try(attr);

      syslog(LOG_INFO, "dma2d: CID %d -> %s\n", cid,
             ret == OK        ? "write landed" :
             ret == -EFAULT   ? "dropped (firewalled)" :
             ret == -ETIMEDOUT ? "timeout" : "xfer error");

      if (ret == OK)
        {
          syslog(LOG_INFO,
                 "dma2d: CID %d grants DMA2D write to SRAM\n", cid);
          return OK;
        }
    }

  syslog(LOG_ERR, "dma2d: no CID 0..7 let DMA2D write SRAM (firewalled)\n");
  return -EFAULT;
}

#endif /* CONFIG_STM32_DMA2D */
