/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_dma.c
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
 * STM32N6 DMA driver for NuttX.
 * Supports GPDMA (General Purpose DMA) for memory/peripheral transfers.
 *
 * Adapted from STM32H7 NuttX reference (stm32_dma.c).
 * Reference: STM32N6 HAL (stm32n6xx_hal_dma.c)
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
#include "stm32n6_dma.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* GPDMA register base addresses */

#define STM32N6_GPDMA1_BASE  0x40021000
#define STM32N6_GPDMA2_BASE  0x40021400

/* Channel 0 register block starts 0x50 past the GPDMA base (per CMSIS
 * GPDMA1_Channel0_BASE = GPDMA1_BASE + 0x50); channels are 0x80 apart.
 * The offsets below are relative to a channel's own base.
 */

#define GPDMA_CH_BASE          0x50
#define GPDMA_CH_STRIDE        0x80

/* GPDMA channel register offsets (per CMSIS DMA_Channel_TypeDef) */

#define GPDMA_CLBAR_OFFSET     0x00
#define GPDMA_CCIDCFGR_OFFSET  0x04
#define GPDMA_CFCR_OFFSET      0x0C
#define GPDMA_CSR_OFFSET       0x10
#define GPDMA_CCR_OFFSET       0x14
#define GPDMA_CTR1_OFFSET      0x40
#define GPDMA_CTR2_OFFSET      0x44
#define GPDMA_CBR1_OFFSET      0x48
#define GPDMA_CSAR_OFFSET      0x4C
#define GPDMA_CDAR_OFFSET      0x50
#define GPDMA_CTR3_OFFSET      0x54
#define GPDMA_CBR2_OFFSET      0x58
#define GPDMA_CLLR_OFFSET      0xCC

/* GPDMA global register offsets */

#define GPDMA_GISR_OFFSET      0x00
#define GPDMA_SECCFGR2_OFFSET  0x24

/* GPDMA_CCR (Channel Control) bits */

#define GPDMA_CCR_EN            (1 << 0)
#define GPDMA_CCR_RESET         (1 << 1)
#define GPDMA_CCR_SUSP          (1 << 2)
#define GPDMA_CCR_TCIE          (1 << 8)   /* Transfer complete IE */
#define GPDMA_CCR_HTIE          (1 << 9)   /* Half transfer IE */
#define GPDMA_CCR_DTEIE         (1 << 10)  /* Data transfer error IE */
#define GPDMA_CCR_ULEIE         (1 << 11)  /* Update link error IE */
#define GPDMA_CCR_USEIE         (1 << 12)  /* User setting error IE */
#define GPDMA_CCR_TOIE          (1 << 13)  /* Trigger overrun IE */
#define GPDMA_CCR_SWRIOIE       (1 << 14)  /* Software request overrun IE */

/* GPDMA_CTR1 (Transfer Register 1) bits: source/destination data width and
 * address increment.  Data width is log2(bytes): 0=byte, 1=half, 2=word.
 */

#define GPDMA_CTR1_SDW_SHIFT   0          /* Source data width (log2 bytes) */
#define GPDMA_CTR1_SINC        (1 << 3)   /* Source address increment */
#define GPDMA_CTR1_DDW_SHIFT   16         /* Destination data width (log2) */
#define GPDMA_CTR1_DINC        (1 << 19)  /* Destination address increment */
#define GPDMA_CTR1_DW_WORD     2          /* 32-bit data width */

/* GPDMA_CTR2 (Transfer Register 2) bits.  Per CMSIS the hardware request
 * selector is REQSEL[7:0]; SWREQ (bit 9) picks software request; DREQ (bit
 * 10) makes the peripheral the *destination* request (default is source,
 * i.e. peripheral-to-memory).
 */

#define GPDMA_CTR2_REQSEL_MASK (0xff << 0)  /* Hardware request line select */
#define GPDMA_CTR2_SWREQ       (1 << 9)     /* Software request */
#define GPDMA_CTR2_DREQ        (1 << 10)    /* Peripheral is destination */
#define GPDMA_CTR2_TCEM_MASK   (3 << 30)    /* Transfer complete event mode */

/* GPDMA_CBR1 (Block Register 1) bits */

#define GPDMA_CBR1_BNDT_MASK   0xFFFF     /* Block number of data bytes */

/* Channel status flags.  Write 1 to clear. */

#define GPDMA_CSR_TCF          (1 << 8)   /* Transfer complete */
#define GPDMA_CSR_HTF          (1 << 9)   /* Half transfer */
#define GPDMA_CSR_DTEF         (1 << 10)  /* Data transfer error */
#define GPDMA_CSR_ULEF         (1 << 11)  /* Update link error */
#define GPDMA_CSR_USEF         (1 << 12)  /* User setting error */
#define GPDMA_CSR_TOF          (1 << 13)  /* Trigger overrun */
#define GPDMA_CSR_SWREQF       (1 << 14)  /* Software request overrun */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_dma_priv_s
{
  uint32_t base;
  uint32_t channel;
  sem_t    lock;
  sem_t    wait;
  bool     initialized;
  uint32_t src_addr;
  uint32_t dst_addr;
  uint32_t block_size;
  bool     complete;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32n6_dma_priv_s g_dma_channels[16];

/* One-shot: the first memory-to-peripheral transfer prints what the channel
 * holds, so a configuration that was silently refused is visible.
 */

static bool g_dma_reported;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline uint32_t dma_getreg(
    struct stm32n6_dma_priv_s *priv, uint32_t offset)
{
  return *(volatile uint32_t *)(priv->base + GPDMA_CH_BASE +
                                priv->channel * GPDMA_CH_STRIDE + offset);
}

static inline void dma_putreg(
    struct stm32n6_dma_priv_s *priv, uint32_t offset,
    uint32_t value)
{
  *(volatile uint32_t *)(priv->base + GPDMA_CH_BASE +
                         priv->channel * GPDMA_CH_STRIDE + offset) = value;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_dma_init(int channel)
{
  struct stm32n6_dma_priv_s *priv;

  if (channel < 0 || channel >= 16)
    {
      return -EINVAL;
    }

  priv = &g_dma_channels[channel];

  if (priv->initialized)
    {
      return 0;
    }

  priv->base = STM32N6_GPDMA1_BASE;
  priv->channel = channel;

  nxsem_init(&priv->lock, 0, 1);
  nxsem_init(&priv->wait, 0, 0);

  /* Open the AHB1 peripheral clock gate feeding GPDMA1.  Without this every
   * channel-register write is silently dropped and reads return zero (proven
   * on real silicon -- Renode does not model clock gating, so this was
   * invisible in simulation).  AHB1ENR is READ-ONLY status on STM32N6;
   * gating is via the AHB1ENSR write-1-to-set alias.
   */

  putreg32(RCC_AHB1ENR_GPDMA1EN, STM32_RCC_AHB1ENSR);

  /* Reset the channel and wait for the hardware to say it is done.  The
   * vendor HAL does this before programming a channel; without it whatever
   * a previous user left in the linked-list pointer and the transfer
   * registers survives into the new configuration.
   */

  dma_putreg(priv, GPDMA_CCR_OFFSET, GPDMA_CCR_RESET);

  for (int i = 0; i < 10000; i++)
    {
      if ((dma_getreg(priv, GPDMA_CCR_OFFSET) & GPDMA_CCR_RESET) == 0)
        {
          break;
        }

      up_udelay(1);
    }

  /* Disable channel */

  dma_putreg(priv, GPDMA_CCR_OFFSET, 0);

  priv->initialized = true;

  syslog(LOG_INFO, "dma: channel %d initialized\n", channel);
  return 0;
}

int stm32n6_dma_start(int channel, uint32_t src, uint32_t dst,
                       uint32_t size)
{
  struct stm32n6_dma_priv_s *priv;

  if (channel < 0 || channel >= 16)
    {
      return -EINVAL;
    }

  priv = &g_dma_channels[channel];

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  nxsem_wait(&priv->lock);

  priv->src_addr = src;
  priv->dst_addr = dst;
  priv->block_size = size;
  priv->complete = false;

  /* Set source address */

  dma_putreg(priv, GPDMA_CSAR_OFFSET, src);

  /* Set destination address */

  dma_putreg(priv, GPDMA_CDAR_OFFSET, dst);

  /* Set block size */

  dma_putreg(priv, GPDMA_CBR1_OFFSET,
             size & GPDMA_CBR1_BNDT_MASK);

  /* Configure transfer: memory-to-memory, software request */

  dma_putreg(priv, GPDMA_CTR2_OFFSET, GPDMA_CTR2_SWREQ);

  /* Enable channel with transfer complete interrupt */

  dma_putreg(priv, GPDMA_CCR_OFFSET,
             GPDMA_CCR_EN | GPDMA_CCR_TCIE);

  nxsem_post(&priv->lock);
  return 0;
}

int stm32n6_dma_start_p2m(int channel, uint32_t paddr, uint32_t maddr,
                          uint32_t size, uint8_t request)
{
  struct stm32n6_dma_priv_s *priv;

  if (channel < 0 || channel >= 16)
    {
      return -EINVAL;
    }

  priv = &g_dma_channels[channel];

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  nxsem_wait(&priv->lock);

  priv->src_addr = paddr;
  priv->dst_addr = maddr;
  priv->block_size = size;
  priv->complete = false;

  /* Source = peripheral (fixed address, word width); destination = memory
   * (incrementing, word width).  A word transfer matches the ADC data
   * register and keeps each sample in its own 32-bit slot.
   */

  dma_putreg(priv, GPDMA_CTR1_OFFSET,
             (GPDMA_CTR1_DW_WORD << GPDMA_CTR1_SDW_SHIFT) |
             (GPDMA_CTR1_DW_WORD << GPDMA_CTR1_DDW_SHIFT) |
             GPDMA_CTR1_DINC);

  /* Hardware request on the given line; peripheral is the source (no DREQ,
   * no SWREQ) so the flow is peripheral-to-memory.
   */

  dma_putreg(priv, GPDMA_CTR2_OFFSET,
             (uint32_t)request & GPDMA_CTR2_REQSEL_MASK);

  dma_putreg(priv, GPDMA_CSAR_OFFSET, paddr);
  dma_putreg(priv, GPDMA_CDAR_OFFSET, maddr);
  dma_putreg(priv, GPDMA_CBR1_OFFSET, size & GPDMA_CBR1_BNDT_MASK);

  /* Enable the channel with transfer-complete interrupt */

  dma_putreg(priv, GPDMA_CCR_OFFSET, GPDMA_CCR_EN | GPDMA_CCR_TCIE);

  nxsem_post(&priv->lock);
  return 0;
}

int stm32n6_dma_start_m2p(int channel, uint32_t maddr, uint32_t paddr,
                          uint32_t size, uint8_t request, uint8_t width)
{
  struct stm32n6_dma_priv_s *priv;

  if (channel < 0 || channel >= 16)
    {
      return -EINVAL;
    }

  priv = &g_dma_channels[channel];

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  nxsem_wait(&priv->lock);

  priv->src_addr = maddr;
  priv->dst_addr = paddr;
  priv->block_size = size;
  priv->complete = false;

  /* Clear the channel's status flags.  They are write-1-to-clear and the
   * previous transfer leaves TCF set, which would make the next poll come
   * back complete before a single byte has moved.
   */

  dma_putreg(priv, GPDMA_CSR_OFFSET, GPDMA_CSR_TCF | GPDMA_CSR_HTF |
                                   GPDMA_CSR_DTEF | GPDMA_CSR_ULEF |
                                   GPDMA_CSR_USEF | GPDMA_CSR_TOF |
                                   GPDMA_CSR_SWREQF);

  /* Source = memory (incrementing); destination = peripheral (fixed).  The
   * width is the caller's: the SAI takes one 16-bit sample per transfer,
   * which is what a WAV file already holds.
   */

  dma_putreg(priv, GPDMA_CTR1_OFFSET,
             ((uint32_t)width << GPDMA_CTR1_SDW_SHIFT) |
             GPDMA_CTR1_SINC |
             ((uint32_t)width << GPDMA_CTR1_DDW_SHIFT));

  /* Hardware request on the given line, peripheral is the destination (DREQ
   * set, no SWREQ) so the flow is memory-to-peripheral.
   */

  dma_putreg(priv, GPDMA_CTR2_OFFSET,
             ((uint32_t)request & GPDMA_CTR2_REQSEL_MASK) |
             GPDMA_CTR2_DREQ);

  dma_putreg(priv, GPDMA_CSAR_OFFSET, maddr);
  dma_putreg(priv, GPDMA_CDAR_OFFSET, paddr);
  dma_putreg(priv, GPDMA_CBR1_OFFSET, size & GPDMA_CBR1_BNDT_MASK);

  /* Enable the channel.  TCIE is set for symmetry with the other two entry
   * points; nothing services it until the IRQ layer is added, so completion
   * is still established by polling.
   */

  dma_putreg(priv, GPDMA_CCR_OFFSET, GPDMA_CCR_EN | GPDMA_CCR_TCIE);

  /* First transfer only: read the channel back and say what it holds.  A
   * value that does not match what was just written means the channel
   * refused the configuration, which on this part means an isolation or
   * ownership problem rather than a programming one.
   */

  if (!g_dma_reported)
    {
      g_dma_reported = true;

      syslog(LOG_INFO, "dma: ch%d m2p m=%08lx p=%08lx n=%lu req=%u\n",
             channel, (unsigned long)maddr, (unsigned long)paddr,
             (unsigned long)size, request);

      syslog(LOG_INFO, "dma: ch%d read back ccr=%08lx ctr1=%08lx ctr2=%08lx "
             "cbr1=%08lx csar=%08lx cdar=%08lx csr=%08lx\n", channel,
             (unsigned long)dma_getreg(priv, GPDMA_CCR_OFFSET),
             (unsigned long)dma_getreg(priv, GPDMA_CTR1_OFFSET),
             (unsigned long)dma_getreg(priv, GPDMA_CTR2_OFFSET),
             (unsigned long)dma_getreg(priv, GPDMA_CBR1_OFFSET),
             (unsigned long)dma_getreg(priv, GPDMA_CSAR_OFFSET),
             (unsigned long)dma_getreg(priv, GPDMA_CDAR_OFFSET),
             (unsigned long)dma_getreg(priv, GPDMA_CSR_OFFSET));
    }

  nxsem_post(&priv->lock);
  return 0;
}

int stm32n6_dma_wait(int channel, int timeout_ms)
{
  struct stm32n6_dma_priv_s *priv;

  if (channel < 0 || channel >= 16)
    {
      return -EINVAL;
    }

  priv = &g_dma_channels[channel];

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  /* Poll for completion against the tick clock.
   *
   * This used to count loop iterations and call them milliseconds.  A
   * register read plus a branch is well under a microsecond, so a "2000 ms"
   * timeout gave up after about 150 ms of real time, which is exactly the
   * kind of thing that makes a driver look broken when it is merely slow.
   *
   * The transfer-complete flag is the signal the hardware guarantees; the
   * channel enable bit also clears on completion in normal mode, and either
   * means the block has landed.
   */

  uint32_t start = clock_systime_ticks();
  uint32_t limit = (uint32_t)MSEC2TICK(timeout_ms);

  for (;;)
    {
      uint32_t cs = dma_getreg(priv, GPDMA_CSR_OFFSET);
      uint32_t cc = dma_getreg(priv, GPDMA_CCR_OFFSET);

      if ((cs & GPDMA_CSR_TCF) != 0 || (cc & GPDMA_CCR_EN) == 0)
        {
          return 0;
        }

      if ((uint32_t)(clock_systime_ticks() - start) >= limit)
        {
          break;
        }
    }

  return -ETIMEDOUT;
}

void stm32n6_dma_deinit(int channel)
{
  struct stm32n6_dma_priv_s *priv;

  if (channel < 0 || channel >= 16)
    {
      return;
    }

  priv = &g_dma_channels[channel];

  if (!priv->initialized)
    {
      return;
    }

  /* Disable channel */

  dma_putreg(priv, GPDMA_CCR_OFFSET, GPDMA_CCR_RESET);

  priv->initialized = false;
  nxsem_destroy(&priv->lock);
  nxsem_destroy(&priv->wait);

  syslog(LOG_INFO, "dma: channel %d deinitialized\n", channel);
}
