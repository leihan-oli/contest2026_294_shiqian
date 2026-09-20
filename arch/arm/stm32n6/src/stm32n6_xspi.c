/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_xspi.c
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
 * STM32N6 XSPI driver for NuttX.
 * Supports XSPI1-2 in memory-mapped mode for NOR Flash.
 * Used for model weight storage and code execution (XIP).
 *
 * Adapted from STM32H7 NuttX reference (stm32_qspi.c).
 * Reference: STM32N6 HAL (stm32n6xx_hal_xspi.c)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/kmalloc.h>
#include <nuttx/semaphore.h>
#include <syslog.h>
#include <string.h>

#include "arm_internal.h"
#include "stm32n6_gpio.h"
#include "stm32n6_xspi.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* XSPI register base addresses (per CMSIS stm32n647xx.h) */

#define STM32N6_XSPI1_BASE  0x48025000
#define STM32N6_XSPI2_BASE  0x4802A000
#define STM32N6_XSPIM_BASE  0x4800B400

/* XSPI register offsets */

#define XSPI_CR_OFFSET      0x00
#define XSPI_DCR1_OFFSET    0x08
#define XSPI_DCR2_OFFSET    0x0C
#define XSPI_DCR3_OFFSET    0x10
#define XSPI_DCR4_OFFSET    0x14
#define XSPI_SR_OFFSET      0x20
#define XSPI_FCR_OFFSET     0x24
#define XSPI_DLR_OFFSET     0x40
#define XSPI_AR_OFFSET      0x48
#define XSPI_DR_OFFSET      0x50
#define XSPI_CCR_OFFSET     0x100
#define XSPI_TCR_OFFSET     0x108
#define XSPI_IR_OFFSET      0x110
#define XSPI_ABR_OFFSET     0x120
#define XSPI_LPTR_OFFSET    0x130
#define XSPI_WPCCR_OFFSET   0x140
#define XSPI_WPTCR_OFFSET   0x148
#define XSPI_WPIR_OFFSET    0x150
#define XSPI_WPABR_OFFSET   0x160
#define XSPI_WCCR_OFFSET    0x180
#define XSPI_WTCR_OFFSET    0x188
#define XSPI_WIR_OFFSET     0x190
#define XSPI_WABR_OFFSET    0x1A0
#define XSPI_HLCR_OFFSET    0x200

/* XSPI_CR bits */

#define XSPI_CR_EN           (1 << 0)
#define XSPI_CR_ABORT        (1 << 1)
#define XSPI_CR_DMAEN        (1 << 2)
#define XSPI_CR_TCEN         (1 << 3)
#define XSPI_CR_DMM          (1 << 6)   /* Dual Memory Mode (CMSIS: XSPI_CR_DMM) */
#define XSPI_CR_FTHRES_SHIFT 8
#define XSPI_CR_FTHRES_MASK  (0x3F << XSPI_CR_FTHRES_SHIFT)
#define XSPI_CR_FMODE_SHIFT  28
#define XSPI_CR_FMODE_MASK   (3 << XSPI_CR_FMODE_SHIFT)

/* CR.FMODE (functional mode).  Only MEMORY_MAPPED puts a transfer engine
 * behind the AXI window; every other value routes the transfers through the
 * indirect path (CCR/TCR/AR/DLR/DR) and leaves the window unanswered.
 */

#define XSPI_CR_FMODE_INDIRECT_WRITE 0
#define XSPI_CR_FMODE_INDIRECT_READ  1
#define XSPI_CR_FMODE_AUTO_POLLING   2
#define XSPI_CR_FMODE_MEMORY_MAPPED  3

/* HyperBus memory-space transaction template.  This is what
 * HAL_XSPI_HyperbusCmd() writes, expanded for this board's part: DQS enabled
 * (the HyperRAM drives RWDS on it), double transfer rate on address and data,
 * eight data lines, 32-bit address, address on eight lines.
 */

#define XSPI_HYPERBUS_TEMPLATE 0x2c003c00

/* XSPI_SR bits */

#define XSPI_SR_TCF          (1 << 1)
#define XSPI_SR_TEF          (1 << 0)
#define XSPI_SR_BUSY         (1 << 5)

/* XSPI_DCR1 bits (CMSIS stm32n647xx.h) */

#define XSPI_DCR1_CKMODE     (1 << 0)
#define XSPI_DCR1_FRCK       (1 << 1)
#define XSPI_DCR1_CSHT_SHIFT 8
#define XSPI_DCR1_CSHT_MASK  (0x3F << XSPI_DCR1_CSHT_SHIFT)
#define XSPI_DCR1_DEVSIZE_SHIFT 16
#define XSPI_DCR1_DEVSIZE_MASK (0x1F << XSPI_DCR1_DEVSIZE_SHIFT)
#define XSPI_DCR1_MTYP_SHIFT 24
#define XSPI_DCR1_MTYP_MASK  (7 << XSPI_DCR1_MTYP_SHIFT)
#define XSPI_DCR1_MTYP_MACRONIX (1 << XSPI_DCR1_MTYP_SHIFT)
#define XSPI_DCR1_MTYP_HYPERBUS (4 << XSPI_DCR1_MTYP_SHIFT)

/* XSPI_DCR2 bits */

#define XSPI_DCR2_PRESCALER_SHIFT 0
#define XSPI_DCR2_PRESCALER_MASK  (0xFF << XSPI_DCR2_PRESCALER_SHIFT)
#define XSPI_DCR2_WRAPSIZE_SHIFT  16
#define XSPI_DCR2_WRAPSIZE_MASK   (7 << XSPI_DCR2_WRAPSIZE_SHIFT)

/* XSPI_DCR3 bits */

#define XSPI_DCR3_MAXTRAN_SHIFT 0
#define XSPI_DCR3_MAXTRAN_MASK  (0xFF << XSPI_DCR3_MAXTRAN_SHIFT)
#define XSPI_DCR3_CSBOUND_SHIFT 16
#define XSPI_DCR3_CSBOUND_MASK  (0x1F << XSPI_DCR3_CSBOUND_SHIFT)

/* XSPI_HLCR bits (HyperBus latency config) */

#define XSPI_HLCR_LM         (1 << 0)
#define XSPI_HLCR_WZL        (1 << 1)
#define XSPI_HLCR_TACC_SHIFT 8
#define XSPI_HLCR_TACC_MASK  (0xFF << XSPI_HLCR_TACC_SHIFT)
#define XSPI_HLCR_TRWR_SHIFT 16
#define XSPI_HLCR_TRWR_MASK  (0xFF << XSPI_HLCR_TRWR_SHIFT)

/* XSPIM_CR bits (IO manager) */

#define XSPIM_CR_CSSEL_OVR_EN (1 << 4)

/* Flash commands (Macronix MX25UM25645G) */

#define NOR_CMD_READ_ID       0x9F
#define NOR_CMD_READ_STATUS   0x05
#define NOR_CMD_WRITE_ENABLE  0x06
#define NOR_CMD_SECTOR_ERASE  0x21
#define NOR_CMD_PAGE_PROGRAM  0x12
#define NOR_CMD_OCTAL_READ    0x0B

/* Memory-mapped mode base addresses
 *   XSPI1 -> HyperRAM (W958D8NBYA5I) @ 0x90000000
 *   XSPI2 -> NOR Flash (MX25UM25645G) @ 0x70000000
 */

#define XSPI1_MMAP_BASE      0x90000000
#define XSPI2_MMAP_BASE      0x70000000

/* Device parameters (MX25UM25645G / W958D8NBYA5I) */

#define XSPI_DEVSIZE_256MB   24   /* DCR1 DEVSIZE: 2^(24+1) = 32 MB */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_xspi_priv_s
{
  uint32_t base;          /* XSPI peripheral register base */
  uint32_t mmap_base;     /* AXI memory-mapped base of the attached memory */
  uint32_t mtyp;          /* DCR1 MTYP (memory type) value */
  uint32_t csht;          /* DCR1 CSHT (chip select high time) in cycles */
  bool     hyperbus;      /* true: HyperBus memory (needs HLCR) */
  FAR const uint32_t *pins; /* GPIO AF9 pin table */
  uint8_t  npins;         /* number of pins in the table */
  sem_t    lock;
  bool     initialized;
  uint32_t flash_size;
  uint32_t sector_size;
  uint32_t page_size;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* XSPI1 pin table — HyperRAM (W958D8NBYA5I), AF9 (FSBL hal_msp.c) */

static const uint32_t g_xspi1_pins[] =
{
  GPIO_XSPI1_IO0, GPIO_XSPI1_IO1, GPIO_XSPI1_IO2, GPIO_XSPI1_IO3,
  GPIO_XSPI1_IO4, GPIO_XSPI1_IO5, GPIO_XSPI1_IO6, GPIO_XSPI1_IO7,
  GPIO_XSPI1_NCS1, GPIO_XSPI1_DQS0, GPIO_XSPI1_CLK, GPIO_XSPI1_NCLK,
};

/* XSPI2 pin table — NOR Flash (MX25UM25645G), AF9 (FSBL hal_msp.c) */

static const uint32_t g_xspi2_pins[] =
{
  GPIO_XSPI2_DQS0, GPIO_XSPI2_NCS1, GPIO_XSPI2_IO0, GPIO_XSPI2_IO1,
  GPIO_XSPI2_IO2, GPIO_XSPI2_IO3, GPIO_XSPI2_CLK, GPIO_XSPI2_IO4,
  GPIO_XSPI2_IO5, GPIO_XSPI2_IO6, GPIO_XSPI2_IO7,
};

static struct stm32n6_xspi_priv_s g_xspi1_priv;
static struct stm32n6_xspi_priv_s g_xspi2_priv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline uint32_t xspi_getreg(
    struct stm32n6_xspi_priv_s *priv, uint32_t offset)
{
  return *(volatile uint32_t *)(priv->base + offset);
}

static inline void xspi_putreg(
    struct stm32n6_xspi_priv_s *priv, uint32_t offset,
    uint32_t value)
{
  *(volatile uint32_t *)(priv->base + offset) = value;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_xspi_dump
 *
 * Description:
 *   Print the controller registers.  Used to record the configuration the
 *   boot ROM / FSBL left behind, which is the known-good one (the FSBL copies
 *   the NuttX image out of the NOR with plain memory-mapped reads).
 *
 ****************************************************************************/

static void stm32n6_xspi_dump(int bus_num)
{
  FAR struct stm32n6_xspi_priv_s *priv;

  switch (bus_num)
    {
      case 1:
        priv = &g_xspi1_priv;
        break;
      case 2:
        priv = &g_xspi2_priv;
        break;
      default:
        return;
    }

  syslog(LOG_INFO,
         "xspi%d: CR=%08lx DCR1=%08lx DCR2=%08lx DCR3=%08lx DCR4=%08lx\n",
         bus_num,
         (unsigned long)xspi_getreg(priv, XSPI_CR_OFFSET),
         (unsigned long)xspi_getreg(priv, XSPI_DCR1_OFFSET),
         (unsigned long)xspi_getreg(priv, XSPI_DCR2_OFFSET),
         (unsigned long)xspi_getreg(priv, XSPI_DCR3_OFFSET),
         (unsigned long)xspi_getreg(priv, XSPI_DCR4_OFFSET));

  syslog(LOG_INFO,
         "xspi%d: CCR=%08lx TCR=%08lx SR=%08lx LPTR=%08lx XSPIM_CR=%08lx"
         " HLCR=%08lx\n",
         bus_num,
         (unsigned long)xspi_getreg(priv, XSPI_CCR_OFFSET),
         (unsigned long)xspi_getreg(priv, XSPI_TCR_OFFSET),
         (unsigned long)xspi_getreg(priv, XSPI_SR_OFFSET),
         (unsigned long)xspi_getreg(priv, XSPI_LPTR_OFFSET),
         (unsigned long)getreg32(STM32N6_XSPIM_BASE + XSPI_CR_OFFSET),
         (unsigned long)xspi_getreg(priv, XSPI_HLCR_OFFSET));
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_xspi_initialize(int bus_num)
{
  FAR struct stm32n6_xspi_priv_s *priv;
  uint32_t regval;
  int i;

  switch (bus_num)
    {
      case 1:
        priv = &g_xspi1_priv;
        priv->base      = STM32N6_XSPI1_BASE;
        priv->mmap_base = XSPI1_MMAP_BASE;
        priv->mtyp      = XSPI_DCR1_MTYP_HYPERBUS;
        priv->csht      = 2;                  /* FSBL ChipSelectHighTime=2 */
        priv->hyperbus  = true;
        priv->pins      = g_xspi1_pins;
        priv->npins     = sizeof(g_xspi1_pins) / sizeof(g_xspi1_pins[0]);
        break;

      case 2:
        priv = &g_xspi2_priv;
        priv->base      = STM32N6_XSPI2_BASE;
        priv->mmap_base = XSPI2_MMAP_BASE;
        priv->mtyp      = XSPI_DCR1_MTYP_MACRONIX;
        priv->csht      = 1;                  /* FSBL ChipSelectHighTime=1 */
        priv->hyperbus  = false;
        priv->pins      = g_xspi2_pins;
        priv->npins     = sizeof(g_xspi2_pins) / sizeof(g_xspi2_pins[0]);
        break;

      default:
        syslog(LOG_ERR, "xspi: unsupported bus %d\n", bus_num);
        return -EINVAL;
    }

  if (priv->initialized)
    {
      return 0;
    }

  nxsem_init(&priv->lock, 0, 1);
  priv->flash_size  = 32 * 1024 * 1024;  /* 256 Mbit */
  priv->sector_size = 4096;
  priv->page_size   = 256;

  /* If the controller is already enabled, the boot ROM (and the FSBL it
   * loaded) has already brought it up, and that configuration is the
   * known-good one: the FSBL copies the NuttX image out of the NOR with plain
   * memory-mapped reads (`destination[i] = source[i]`), so the moment we run,
   * the window is proven to work.
   *
   * Re-programming it here is what broke the window: the single 32-bit write
   * this driver used to do to the XSPIM chip select register (`XSPIM->CR =
   * CSSEL_OVR_EN`) replaced the boot ROM's IO-manager routing and arbitration
   * settings with a bare override, and every access to 0x70000000 then came
   * back as a precise bus error (CFSR.PRECISERR, BFAR = 0x70200000) - for the
   * CPU and, because the NPU reads its weights from the same window, for the
   * NPU as well.
   *
   * Leave everything alone and just record the registers so the configuration
   * can be replicated later if NuttX ever has to bring the controller up by
   * itself.
   */

  if ((xspi_getreg(priv, XSPI_CR_OFFSET) & XSPI_CR_EN) != 0)
    {
      stm32n6_xspi_dump(bus_num);

      /* The boot ROM arms both controllers, but it only ever needs to read
       * the boot image - and that lives on the NOR (XSPI2).  It probes the
       * HyperRAM (XSPI1) with indirect reads and then leaves the controller
       * in that mode: CR.FMODE reads INDIRECT_READ.
       *
       * FMODE is what decides whether the AXI window has a transfer engine
       * behind it.  Only MEMORY_MAPPED does; with the controller left in
       * indirect mode every access to 0x90000000 is unanswered and comes back
       * as an "Imprecise data bus error" (R0=90000014).  The device
       * parameters themselves are already right - DCR1 carries
       * MTYP=HYPERBUS and DEVSIZE=24, DCR2 matches the reference
       * configuration - so this only applies the two things the boot ROM has
       * no use for: the HyperBus latency and the functional mode.
       *
       * The memory mapping, the IO routing (XSPIM is shared with the NOR and
       * was already broken once by rewriting it) and the NOR controller are
       * all left alone.
       */

      if (priv->hyperbus)
        {
          uint32_t fmode;
          int retries;

          /* The boot ROM arms both controllers, but it only ever looked for a
           * boot image - and that is on the NOR (XSPI2).  For the HyperRAM it
           * probed XSPI1 the way it probes any candidate flash, then walked
           * away with the controller still in indirect-read mode and a
           * flash-style transaction template left in CCR (IR = 0x0b, eight
           * dummy cycles).  Nothing was ever written to the write registers.
           *
           * Two things make the window usable, both taken from this board's
           * own HyperRAM_Init()/HAL_XSPI_HyperbusCmd() bring-up:
           *
           *   1. memory-mapped loads go through CCR/TCR/IR/ABR and
           *      memory-mapped stores through WCCR/WTCR/WIR/WABR.  The write
           *      set was all zero, so every store to the window was rejected
           *      with an imprecise data bus error while loads merely returned
           *      mis-sampled data - which is why a plain memset() into the
           *      window was the thing that always faulted;
           *   2. the transaction registers only accept writes while FMODE is
           *      0, and the boot ROM had already left the controller in
           *      indirect-read mode.
           *
           * Verified on the board: after this, a store of 0x12345678 to
           * 0x90000000 reads back as 0x12345678.
           */

          /* Let any transfer the boot ROM left running finish first */

          for (retries = 0; retries < 1000; retries++)
            {
              if ((xspi_getreg(priv, XSPI_SR_OFFSET) & XSPI_SR_BUSY) == 0)
                {
                  break;
                }

              up_udelay(1);
            }

          /* 1. Leave memory-mapped mode, or the registers below are ignored */

          regval  = xspi_getreg(priv, XSPI_CR_OFFSET);
          fmode   = (regval & XSPI_CR_FMODE_MASK) >> XSPI_CR_FMODE_SHIFT;
          regval &= ~XSPI_CR_FMODE_MASK;
          xspi_putreg(priv, XSPI_CR_OFFSET, regval);

          /* 2. No wrapped bursts.  The board configuration asks for 32-byte
           *    wraps (DCR2.WRAPSIZE = 3) and the boot ROM copied it, but this
           *    device does not honour it: reads come back rotated inside the
           *    wrap boundary and nothing reports an error - the contents just
           *    silently come back wrong, which is what turned a heap walk into
           *    a write to a wild address.  The NOR controller, which works,
           *    runs with WRAPSIZE = 0; match it.
           */

          xspi_putreg(priv, XSPI_DCR2_OFFSET, 0);

          /* 3. The same HyperBus memory-space template in both register sets */

          xspi_putreg(priv, XSPI_CCR_OFFSET, XSPI_HYPERBUS_TEMPLATE);
          xspi_putreg(priv, XSPI_WCCR_OFFSET, XSPI_HYPERBUS_TEMPLATE);

          /* 4. HyperBus latency: TRWR = 7, TACC = 7, fixed latency, latency
           *    on write - the values the board BSP programs.
           */

          xspi_putreg(priv, XSPI_HLCR_OFFSET, 0x00070701);

          /* 5. Arm the timeout counter and go back to memory-mapped mode.
           *    TCEN is what turns an unanswered access into an error instead
           *    of holding the AXI transaction open forever.
           */

          xspi_putreg(priv, XSPI_LPTR_OFFSET, 0x34);
          xspi_putreg(priv, XSPI_CR_OFFSET,
                      XSPI_CR_EN | XSPI_CR_TCEN |
                      (3 << XSPI_CR_FTHRES_SHIFT) |
                      (XSPI_CR_FMODE_MEMORY_MAPPED << XSPI_CR_FMODE_SHIFT));

          syslog(LOG_INFO,
                 "xspi%d: HyperRAM armed (FMODE %lu -> %lu, CCR=%08lx"
                 " WCCR=%08lx DCR2=%08lx HLCR=%08lx CR=%08lx)\n",
                 bus_num, (unsigned long)fmode,
                 (unsigned long)XSPI_CR_FMODE_MEMORY_MAPPED,
                 (unsigned long)xspi_getreg(priv, XSPI_CCR_OFFSET),
                 (unsigned long)xspi_getreg(priv, XSPI_WCCR_OFFSET),
                 (unsigned long)xspi_getreg(priv, XSPI_DCR2_OFFSET),
                 (unsigned long)xspi_getreg(priv, XSPI_HLCR_OFFSET),
                 (unsigned long)xspi_getreg(priv, XSPI_CR_OFFSET));
        }

      priv->initialized = true;

      syslog(LOG_INFO,
             "xspi%d: enabled and memory-mapped by the boot ROM%s\n",
             bus_num, priv->hyperbus ? ", HyperRAM window armed" :
                                       ", leaving it untouched");
      return 0;
    }

  /* 1. Enable the peripheral clocks: XSPI1/2 + XSPIM (AHB5ENR).
   *    The XSPI kernel clock source is HCLK (RCC_XSPIxCLKSOURCE_HCLK,
   *    selected in the FSBL; reset default is HCLK).
   */

  regval  = getreg32(STM32_RCC_BASE + STM32_RCC_AHB5ENR_OFFSET);
  regval |= RCC_AHB5ENR_XSPIMEN;
  regval |= (bus_num == 1) ? RCC_AHB5ENR_XSPI1EN : RCC_AHB5ENR_XSPI2EN;
  putreg32(regval, STM32_RCC_BASE + STM32_RCC_AHB5ENR_OFFSET);

  /* 2. Configure the GPIO pins (AF9, push-pull, high speed, no pull).
   *    The XSPI I/O voltage domains (VDDIO2/3) are already set to 1.8V
   *    by stm32n6_start() via stm32n6_pwr_enablevddio(BOARD_PWR_VDDIO).
   */

  for (i = 0; i < priv->npins; i++)
    {
      stm32n6_configgpio(priv->pins[i]);
    }

  /* 3. Disable XSPI before (re)configuring the device */

  xspi_putreg(priv, XSPI_CR_OFFSET, 0);

  /* 4. Device configuration:
   *    DCR1: memory type + 256 Mbit size (DEVSIZE=24 -> 2^25 = 32 MB)
   *          + chip-select high time + clock mode 0.
   *    DCR2: prescaler 0 (kernel clock / 1) + no wrap (P1 may refine).
   *    DCR3: no CS boundary, no max transfer limit.
   *    DCR4: no refresh.
   */

  xspi_putreg(priv, XSPI_DCR1_OFFSET,
              priv->mtyp |
              (XSPI_DEVSIZE_256MB << XSPI_DCR1_DEVSIZE_SHIFT) |
              ((priv->csht - 1) << XSPI_DCR1_CSHT_SHIFT));

  xspi_putreg(priv, XSPI_DCR2_OFFSET, 0);
  xspi_putreg(priv, XSPI_DCR3_OFFSET, 0);
  xspi_putreg(priv, XSPI_DCR4_OFFSET, 0);

  /* 5. HyperBus latency config (XSPI1 / HyperRAM only).
   *    FSBL: RWRecovery=7, AccessTime=7, latency on write, fixed latency.
   *    HLCR = (TRWR<<16) | (TACC<<8) | WZL | LM = 0x00070701
   */

  if (priv->hyperbus)
    {
      xspi_putreg(priv, XSPI_HLCR_OFFSET, 0x00070701);
    }

  /* 6. XSPIM IO manager: route the chip select to NCS1 (CSSEL_OVR_EN).
   *    IO port selection is implicit in the pin mux (XSPI1=IO port 1,
   *    XSPI2=IO port 2); no MUXEN/MODE needed for non-shared ports.
   */

  putreg32(XSPIM_CR_CSSEL_OVR_EN, STM32N6_XSPIM_BASE + XSPI_CR_OFFSET);

  /* 7. Memory-mapped mode timeout (low-power entry counter) */

  xspi_putreg(priv, XSPI_LPTR_OFFSET, 0x400);

  /* 8. Enable XSPI with FIFO threshold of 4 bytes (FTHRES = 4 - 1) */

  xspi_putreg(priv, XSPI_CR_OFFSET,
              (3 << XSPI_CR_FTHRES_SHIFT) | XSPI_CR_EN);

  priv->initialized = true;

  syslog(LOG_INFO, "xspi%d: initialized, mapped @ 0x%08lx\n",
         bus_num, (unsigned long)priv->mmap_base);

  return 0;
}

int stm32n6_xspi_enable_mmap(int bus_num)
{
  struct stm32n6_xspi_priv_s *priv;

  switch (bus_num)
    {
      case 1:
        priv = &g_xspi1_priv;
        break;
      case 2:
        priv = &g_xspi2_priv;
        break;
      default:
        return -EINVAL;
    }

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  /* There is nothing for the CPU to enable here: the memory-mapped window is
   * armed by the configuration the boot ROM programs into the controller
   * (CCR/TCR describe the flash protocol, XSPIM CR the IO routing).  Writing
   * CR.DMM ("dual memory mode", which an earlier version of this driver
   * mistook for "memory mapped write enable") would put the controller into a
   * mode that expects two devices.
   */

  if ((xspi_getreg(priv, XSPI_CR_OFFSET) & XSPI_CR_EN) == 0)
    {
      syslog(LOG_ERR, "xspi%d: controller is disabled, window not mapped\n",
             bus_num);
      return -EIO;
    }

  syslog(LOG_INFO, "xspi%d: memory-mapped window is active\n", bus_num);
  return 0;
}

int stm32n6_xspi_read(int bus_num, uint32_t offset,
                       void *buf, uint32_t len)
{
  FAR struct stm32n6_xspi_priv_s *priv;

  switch (bus_num)
    {
      case 1:
        priv = &g_xspi1_priv;
        break;
      case 2:
        priv = &g_xspi2_priv;
        break;
      default:
        return -EINVAL;
    }

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  if (offset + len > priv->flash_size)
    {
      return -EINVAL;
    }

  /* Memory-mapped read (the bus must have been put into memory-mapped
   * mode first; on XSPI2 this additionally requires the NOR command
   * sequence / PHY configuration from P1).
   */

  memcpy(buf, (FAR void *)(priv->mmap_base + offset), len);
  return len;
}

void stm32n6_xspi_deinit(int bus_num)
{
  struct stm32n6_xspi_priv_s *priv;

  switch (bus_num)
    {
      case 1:
        priv = &g_xspi1_priv;
        break;
      case 2:
        priv = &g_xspi2_priv;
        break;
      default:
        return;
    }

  if (!priv->initialized)
    {
      return;
    }

  xspi_putreg(priv, XSPI_CR_OFFSET, 0);
  priv->initialized = false;
  nxsem_destroy(&priv->lock);

  syslog(LOG_INFO, "xspi%d: deinitialized\n", bus_num);
}
