/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_sai.c
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
 * STM32N6 SAI driver for NuttX.
 * Serial Audio Interface for microphone input (MEMS mic via I2S).
 *
 * Adapted from STM32F7 NuttX reference (stm32_sai.c).
 * Reference: STM32N6 HAL (stm32n6xx_hal_sai.c)
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/arch.h>
#include <nuttx/kmalloc.h>
#include <nuttx/semaphore.h>
#include <syslog.h>
#include <string.h>

#include "arm_internal.h"
#include "stm32n6_dma.h"
#include "stm32n6_gpio.h"
#include "stm32n6_sai.h"

/* memorymap first: the RCC header derives every register address from
 * STM32_RCC_BASE, which that header owns.
 */

#include "hardware/stm32_memorymap.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* SAI1 base address (per CMSIS stm32n647xx.h). The driver's register
 * offsets below (SAI_CR1_OFFSET=0x04, etc.) are relative to this base
 * and already encode the CMSIS Block A control-register placement
 * (SAI_Block_TypeDef starting at SAI1_BASE + 0x04), matching the
 * Renode STM32N6_SAI model's absolute offsets from the sai1 base.
 */

#define STM32N6_SAI1_BASE   0x42005800

/* SAI register offsets (per block: 0x20 spacing) */

#define SAI_CR1_OFFSET      0x04
#define SAI_CR2_OFFSET      0x08
#define SAI_FRCR_OFFSET     0x0C
#define SAI_SLOTR_OFFSET    0x10
#define SAI_IMR_OFFSET      0x14
#define SAI_SR_OFFSET       0x18
#define SAI_CLRFR_OFFSET    0x1C
#define SAI_DR_OFFSET       0x20

/* SAI_CR1 bits */

#define SAI_CR1_SAIEN       (1 << 16)
#define SAI_CR1_DMAEN       (1 << 17)
#define SAI_CR1_MODE_MASK   (3 << 0)
#define SAI_CR1_MODE_RX     (2 << 0)
#define SAI_CR1_DS_MASK     (7 << 5)
#define SAI_CR1_DS_16BIT    (4 << 5)
#define SAI_CR1_DS_24BIT    (6 << 5)
#define SAI_CR1_LSBFIRST    (1 << 12)
#define SAI_CR1_CKSTR       (1 << 9)
#define SAI_CR1_MCKDIV_MASK (0x3F << 20)

/* SAI_FRCR bits (frame control) */

#define SAI_FRCR_FRL_MASK   (0xFF << 0)  /* Frame length */
#define SAI_FRCR_FSALL_MASK (0x3F << 8)  /* FS active level length */

/* SAI_SLOTR bits */

#define SAI_SLOTR_FBOFF_MASK (0x1F << 0)
#define SAI_SLOTR_SLOTSZ_MASK (3 << 6)
#define SAI_SLOTR_SLOTSZ_16B  (1 << 6)  /* one 16-bit sample per slot */
#define SAI_SLOTR_NBSLOT_MASK (0xF << 8)
#define SAI_SLOTR_SLOTEN_MASK (0xFFFF << 16)

/* SAI status bits */

#define SAI_SR_FREQ         (1 << 3)
#define SAI_SR_OVRUDR       (1 << 0)

/* Audio constants */

#define SAI_DEFAULT_MCLK_DIV  4  /* MCLK = HCLK / 4 */

/* --- playback ------------------------------------------------------------ */

/* Register fields the input path never touched.  Transcribed from the SAI
 * block's definitions rather than guessed: CKSTR selects which edge data
 * changes on, MCKEN drives MCLK out to the codec, and FSDEF/FSOFF describe
 * how the frame sync relates to the first bit.
 */

#define SAI_CR1_CKSTR        (1 << 9)    /* data changes on falling edge (I2S) */
#define SAI_CR1_OUTDRIV     (1 << 13)    /* output drive enabled */
#define SAI_CR1_SAIEN       (1 << 16)    /* block enable */
#define SAI_CR1_MCKEN       (1 << 27)    /* master clock generation + output */
#define SAI_FRCR_FSDEF      (1 << 16)    /* FS = channel identification */
#define SAI_FRCR_FSOFF      (1 << 18)    /* FS starts before the first bit */
#define SAI_CR2_FTH_1QF     (1 << 0)     /* FIFO threshold 1/4 */
#define SAI_CR1_MCKDIV(d)   (((d) & 0x3f) << 20)  /* master clock divider */

#define SAI_TX_FRAME_BITS  32            /* 16-bit sample x 2 slots */
#define SAI_TX_ACTIVE_BITS 16
#define SAI_TX_SLOTS       2

#define SAI_TX_DMA_CHAN    1
#define SAI_TX_DMA_REQUEST 91            /* GPDMA1 request line: SAI1_A */

/* RCC bits this path needs.  The port's RCC header lists IC1/2/3/6/11/16/17/18
 * because those are the ones existing drivers use; SAI1 hangs off IC7, which
 * was not among them.  Offsets are the CMSIS struct's.
 */

#define STM32N6_RCC_IC7CFGR      (STM32_RCC_BASE + 0x00dc)

/* PLL reference-source encodings.  The port's RCC header has
 * RCC_PLL1CFGR1_SEL_HSE as (1 << 28), but the vendor HAL writes (2 << 28)
 * for HSE and uses (1 << 28) for MSI, with 0 meaning HSI.  PLL1 on this board
 * reads back SEL = 0 and runs at 64/4*75 = 1200 MHz, which confirms 0 is HSI
 * and therefore that the header's value is wrong.
 */

#define RCC_PLLSEL_HSI           (0 << 28)
#define RCC_PLLSEL_HSE           (2 << 28)
#define RCC_PLL2CFGR1_SEL_MASK   (7 << 28)
#define RCC_PLL2CFGR1_BYP        (1 << 27)
#define RCC_PLL2CFGR1_DIVM_MASK  (0x3f << 20)
#define RCC_PLL2CFGR1_DIVN_MASK  (0xfff << 8)
#define RCC_PLL2CFGR3_PDIV1_MASK (7 << 27)
#define RCC_PLL2CFGR3_PDIV2_MASK (7 << 24)
#define RCC_PLL2CFGR2_FRAC_MASK  (0xffffff)

/* The port's RCC header defines PLL2CFGR1 and PLL2CFGR3 but not CFGR2, which
 * holds the fractional part of DIVN.  Playback runs in integer mode and only
 * ever clears it, so it is spelled out here rather than added to the shared
 * header for a single zero write.
 */

#define STM32N6_RCC_PLL2CFGR2    (STM32_RCC_BASE + 0x0094)
#define RCC_DIVENR_IC7EN         (1 << 6)
#define RCC_APB2ENR_SAI1EN       (1 << 21)

/* PLL2, transcribed from the vendor BSP's 44.1 kHz case.  PLL2 is off on this
 * board and nothing else claims it: the NPU's IC6 stays on PLL1 by a
 * deliberate earlier decision, so borrowing PLL2 here disturbs nothing.
 */

#define SAI_PLL2_DIVM            (6)
#define SAI_PLL2_DIVN            (192)
#define SAI_PLL2_PDIV1           (4)
#define SAI_PLL2_PDIV2           (2)
#define SAI_IC7_DIV              (17)
#define SAI_HSE_HZ               (48000000)

/* Kernel clock reaching SAI1, and the divider its block needs.
 *
 * PLL2 drives IC7 and IC7 drives the block: 48 MHz / 6 * 192 / 4 / 17, which
 * comes to 22.58 MHz.  With NODIV clear the block's master clock is the
 * kernel clock divided by MCKDIV, one audio frame lasts 256 master clocks,
 * and so the divider that puts a frame out at the sample rate is
 *
 *   MCKDIV = kernel clock / (256 * Fs)
 *
 * which is 2 here, and is the same value the vendor HAL computes for the
 * same clock and the same request.  Zero is not a valid divider: the master
 * clock never runs, no frame is ever produced, the FIFO is never drained and
 * the request line is never asserted - the fill that had already reached the
 * FIFO just sits there, which is what a timeout looks like from above.
 */

#define SAI_TX_KERNEL_HZ \
  (SAI_HSE_HZ / SAI_PLL2_DIVM * SAI_PLL2_DIVN / SAI_PLL2_PDIV1 / SAI_IC7_DIV)

#define SAI_TX_RATE_HZ          (44100)
#define SAI_TX_MCKDIV           (SAI_TX_KERNEL_HZ / (SAI_TX_RATE_HZ * 256))

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_sai_priv_s
{
  uint32_t base;
  sem_t    lock;
  sem_t    dma_wait;
  bool     initialized;
  struct sai_config_s config;
  sai_buffer_cb_t callback;
  void    *cb_arg;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32n6_sai_priv_s g_sai1a_priv;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline uint32_t sai_getreg(
    struct stm32n6_sai_priv_s *priv, uint32_t offset)
{
  return *(volatile uint32_t *)(priv->base + offset);
}

static inline void sai_putreg(
    struct stm32n6_sai_priv_s *priv, uint32_t offset,
    uint32_t value)
{
  *(volatile uint32_t *)(priv->base + offset) = value;
}

static int sai_config_cr1(struct stm32n6_sai_priv_s *priv)
{
  uint32_t cr1 = 0;

  /* Mode: receiver */

  cr1 |= SAI_CR1_MODE_RX;

  /* Data size */

  if (priv->config.bits_per_sample == 24)
    {
      cr1 |= SAI_CR1_DS_24BIT;
    }
  else
    {
      cr1 |= SAI_CR1_DS_16BIT;
    }

  /* MCLK divider */

  cr1 |= (SAI_DEFAULT_MCLK_DIV << 20) &
          SAI_CR1_MCKDIV_MASK;

  /* Enable DMA */

  cr1 |= SAI_CR1_DMAEN;

  sai_putreg(priv, SAI_CR1_OFFSET, cr1);
  return 0;
}

static void sai_config_frame(struct stm32n6_sai_priv_s *priv)
{
  uint32_t frcr;
  uint32_t slotr;
  uint32_t frame_len;
  uint32_t slot_count;

  if (priv->config.channels == 2)
    {
      frame_len = priv->config.bits_per_sample * 2;
      slot_count = 2;
    }
  else
    {
      frame_len = priv->config.bits_per_sample;
      slot_count = 1;
    }

  /* Frame length and FS active level */

  frcr = ((frame_len - 1) & 0xff) |
         (((priv->config.bits_per_sample - 1) & 0x3f) << 8);
  sai_putreg(priv, SAI_FRCR_OFFSET, frcr);

  /* Slot configuration */

  slotr = (((priv->config.bits_per_sample - 1) & 0x3) << 6) |
          (((slot_count - 1) & 0xf) << 8) |
          (((1 << slot_count) - 1) << 16);
  sai_putreg(priv, SAI_SLOTR_OFFSET, slotr);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_sai_input_init(const struct sai_config_s *config)
{
  struct stm32n6_sai_priv_s *priv = &g_sai1a_priv;

  if (priv->initialized)
    {
      return 0;
    }

  priv->base = STM32N6_SAI1_BASE;
  priv->config = *config;

  nxsem_init(&priv->lock, 0, 1);
  nxsem_init(&priv->dma_wait, 0, 0);

  /* Disable SAI */

  sai_putreg(priv, SAI_CR1_OFFSET, 0);

  /* Configure CR1 */

  sai_config_cr1(priv);

  /* Configure frame and slot */

  sai_config_frame(priv);

  priv->initialized = true;

  syslog(LOG_INFO, "sai: initialized %luHz %ubit %uch\n",
         (unsigned long)config->sample_rate,
         config->bits_per_sample,
         config->channels);
  return 0;
}

int stm32n6_sai_input_start(sai_buffer_cb_t callback,
                              void *arg)
{
  struct stm32n6_sai_priv_s *priv = &g_sai1a_priv;

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  priv->callback = callback;
  priv->cb_arg = arg;

  /* Enable SAI */

  sai_putreg(priv, SAI_CR1_OFFSET,
             sai_getreg(priv, SAI_CR1_OFFSET) |
             SAI_CR1_SAIEN);

  syslog(LOG_INFO, "sai: capture started\n");
  return 0;
}

int stm32n6_sai_input_stop(void)
{
  struct stm32n6_sai_priv_s *priv = &g_sai1a_priv;

  if (!priv->initialized)
    {
      return -ENODEV;
    }

  /* Disable SAI */

  sai_putreg(priv, SAI_CR1_OFFSET,
             sai_getreg(priv, SAI_CR1_OFFSET) &
             ~SAI_CR1_SAIEN);

  priv->callback = NULL;
  priv->cb_arg = NULL;

  syslog(LOG_INFO, "sai: capture stopped\n");
  return 0;
}

void stm32n6_sai_input_deinit(void)
{
  struct stm32n6_sai_priv_s *priv = &g_sai1a_priv;

  if (!priv->initialized)
    {
      return;
    }

  stm32n6_sai_input_stop();

  sai_putreg(priv, SAI_CR1_OFFSET, 0);

  priv->initialized = false;
  nxsem_destroy(&priv->lock);
  nxsem_destroy(&priv->dma_wait);

  syslog(LOG_INFO, "sai: deinitialized\n");
}


/* --- playback ------------------------------------------------------------ *
 *
 * SAI1 block A as master transmitter.  The codec is the slave and takes both
 * its bit clock and its MCLK from here, so the whole audio path hangs off
 * this block's kernel clock.
 */

/****************************************************************************
 * Name: stm32n6_sai_clock_config
 *
 * Description:
 *   Point SAI1's kernel clock at IC7 and start PLL2 feeding it.
 *
 *   PLL2 is off on this board and nothing else claims it: the NPU's IC6 was
 *   deliberately left on PLL1 (crossing to a second PLL cost more than the
 *   frequency bought back), and every other kernel clock in use is routed
 *   elsewhere.  So this is additive.
 *
 ****************************************************************************/

static int stm32n6_sai_clock_config(void)
{
  uint32_t regval;
  int      i;

  /* HSE first.
   *
   * The vendor's PLL2 settings are built around a 48 MHz reference.  This
   * board's PLL1 runs from HSI instead - IC6 and the NPU domain are clocked
   * from PLL1 at 1200 MHz, which is 64/4*75 and only works out from HSI - so
   * HSE is off at boot and has to be started before it can feed anything.
   *
   * Only PLL2 is taken from it.  PLL1, and with it SYSCLK, the NPU and every
   * other kernel clock, is left exactly as the FSBL set it up.
   */

  putreg32(getreg32(STM32_RCC_CR) | RCC_CR_HSEON, STM32_RCC_CR);

  for (i = 0; i < 100000; i++)
    {
      if ((getreg32(STM32_RCC_SR) & RCC_SR_HSERDY) != 0)
        {
          break;
        }

      up_udelay(1);
    }

  if (i >= 100000)
    {
      syslog(LOG_ERR, "sai: HSE never started (SR=%08lx)\n",
             (unsigned long)getreg32(STM32_RCC_SR));
      return -ETIMEDOUT;
    }

  /* PLL2: source, dividers and post-dividers.  Read-modify-write on exactly
   * the fields the vendor HAL touches, so whatever else the FSBL left in
   * these registers survives - a blind write would clear bits nobody here
   * understands.  BYP goes with them: it is set at reset, and a bypassed PLL
   * ignores DIVM and DIVN entirely.
   */

  regval  = getreg32(STM32_RCC_PLL2CFGR1);
  regval &= ~(RCC_PLL2CFGR1_SEL_MASK | RCC_PLL2CFGR1_BYP |
              RCC_PLL2CFGR1_DIVM_MASK | RCC_PLL2CFGR1_DIVN_MASK);
  regval |= RCC_PLLSEL_HSE |
            ((uint32_t)SAI_PLL2_DIVM << 20) |
            ((uint32_t)SAI_PLL2_DIVN << 8);
  putreg32(regval, STM32_RCC_PLL2CFGR1);

  regval  = getreg32(STM32N6_RCC_PLL2CFGR2);
  putreg32(regval & ~RCC_PLL2CFGR2_FRAC_MASK, STM32N6_RCC_PLL2CFGR2);

  regval  = getreg32(STM32_RCC_PLL2CFGR3);
  regval &= ~(RCC_PLL2CFGR3_PDIV1_MASK | RCC_PLL2CFGR3_PDIV2_MASK);
  regval |= ((uint32_t)SAI_PLL2_PDIV1 << 27) |
            ((uint32_t)SAI_PLL2_PDIV2 << 24);
  putreg32(regval, STM32_RCC_PLL2CFGR3);

  /* Enable it.  Nothing locks before this: the previous version configured
   * a PLL that was never switched on, so the wait could only time out.
   */

  putreg32(getreg32(STM32_RCC_CR) | RCC_CR_PLL2ON, STM32_RCC_CR);

  for (i = 0; i < 100000; i++)
    {
      if ((getreg32(STM32_RCC_SR) & RCC_SR_PLL2RDY) != 0)
        {
          break;
        }

      up_udelay(1);
    }

  if (i >= 100000)
    {
      syslog(LOG_ERR, "sai: PLL2 never locked (SR=%08lx)\n",
             (unsigned long)getreg32(STM32_RCC_SR));
      return -ETIMEDOUT;
    }

  /* IC7 takes PLL2.  Its integer field holds the divider *minus one*, which
   * is what the vendor HAL writes.
   */

  putreg32(RCC_ICCFGR_SEL_PLL2 |
           ((uint32_t)(SAI_IC7_DIV - 1) << RCC_ICCFGR_INT_SHIFT),
           STM32N6_RCC_IC7CFGR);

  /* IC dividers have their own gate, off at reset.  Write-1-to-set alias. */

  putreg32(RCC_DIVENR_IC7EN, STM32_RCC_DIVENSR);

  /* And SAI1's APB2 gate. */

  putreg32(RCC_APB2ENR_SAI1EN, STM32_RCC_APB2ENSR);

  return 0;
}

/****************************************************************************
 * Name: stm32n6_sai_output_apply
 *
 * Description:
 *   Program the block.  Frame length 32 bits with 16 active, two 16-bit
 *   slots, FS before the first bit and active low: the standard I2S shape,
 *   and the one the vendor BSP uses.
 *
 *   NODIV stays clear, so the master clock comes from MCKDIV and works
 *   out at 256 * Fs: one frame per 256 master clocks, at 44.1 kHz.
 *
 ****************************************************************************/

static void stm32n6_sai_output_apply(struct stm32n6_sai_priv_s *priv)
{
  uint32_t cr1;
  uint32_t frcr;
  uint32_t slotr;

  cr1 = SAI_CR1_CKSTR | SAI_CR1_OUTDRIV | SAI_CR1_DMAEN | SAI_CR1_MCKEN |
        SAI_CR1_DS_16BIT |
        SAI_CR1_MCKDIV(SAI_TX_MCKDIV);

  frcr = ((SAI_TX_FRAME_BITS - 1) & 0xff) |
         SAI_FRCR_FSDEF | SAI_FRCR_FSOFF |
         (((SAI_TX_ACTIVE_BITS - 1) & 0x3f) << 8);

  slotr = SAI_SLOTR_SLOTSZ_16B |
          (((SAI_TX_SLOTS - 1) & 0xf) << 8) |
          (((1 << SAI_TX_SLOTS) - 1) << 16);

  /* Off while it is being programmed, then on.  MCLK starts with SAIEN, so
   * the codec is listening before the first sample arrives.
   */

  sai_putreg(priv, SAI_CR1_OFFSET, 0);
  sai_putreg(priv, SAI_CR2_OFFSET, SAI_CR2_FTH_1QF);
  sai_putreg(priv, SAI_FRCR_OFFSET, frcr);
  sai_putreg(priv, SAI_SLOTR_OFFSET, slotr);
  sai_putreg(priv, SAI_CR1_OFFSET, cr1 | SAI_CR1_SAIEN);
}

int stm32n6_sai_output_init(uint32_t sample_rate, uint8_t channels)
{
  struct stm32n6_sai_priv_s *priv = &g_sai1a_priv;
  int ret;

  /* Only the 44.1 kHz family has clock numbers transcribed from the vendor
   * BSP; every other rate wants a different PLL2 setting and divider.
   */

  if (sample_rate != 44100 || channels != 2)
    {
      syslog(LOG_ERR, "sai: playback is wired for 44.1 kHz stereo only\n");
      return -EINVAL;
    }

  priv->base = STM32N6_SAI1_BASE;
  priv->config.sample_rate    = sample_rate;
  priv->config.channels       = channels;
  priv->config.bits_per_sample = 16;

  ret = stm32n6_sai_clock_config();
  if (ret < 0)
    {
      /* No clock means no MCLK, which means no acknowledge from the codec
       * and a transmit block that never asks the DMA for anything.  Failing
       * here says why, instead of leaving a timeout three layers down.
       */

      return ret;
    }

  stm32n6_configgpio(GPIO_SAI1_FS_A);
  stm32n6_configgpio(GPIO_SAI1_SCK_A);
  stm32n6_configgpio(GPIO_SAI1_SD_A);
  stm32n6_configgpio(GPIO_SAI1_MCLK_A);

  stm32n6_sai_output_apply(priv);

  stm32n6_dma_init(SAI_TX_DMA_CHAN);

  priv->initialized = true;

  stm32n6_sai_output_dump();
  return 0;
}

/* Set once the channel has failed to complete a block.  Everything after
 * that goes out through the data register, which needs no request line.
 */

static bool g_sai_cpu_feed;

/****************************************************************************
 * Name: stm32n6_sai_output_feed
 *
 * Description:
 *   Hand the block two samples at a time, one frame per data register write.
 *
 *   The FIFO drains at the sample rate, so the hardware paces this by
 *   itself: a write is only made when there is room, and room appears 44100
 *   times a second.  Slower than the channel and it holds the CPU for the
 *   whole file, but it needs no request line, no channel and no memory
 *   attributes, so audio plays even when the channel does not complete.
 *
 ****************************************************************************/

static int stm32n6_sai_output_feed(FAR const int16_t *samples, uint32_t count)
{
  struct stm32n6_sai_priv_s *priv = &g_sai1a_priv;
  uint32_t last = clock_systime_ticks();
  uint32_t i    = 0;

  while (i < count)
    {
      if (((sai_getreg(priv, SAI_SR_OFFSET) >> 16) & 0xf) <= 4)
        {
          uint32_t word = (uint32_t)(uint16_t)samples[i];

          i++;

          if (i < count)
            {
              word |= (uint32_t)(uint16_t)samples[i] << 16;
              i++;
            }

          sai_putreg(priv, SAI_DR_OFFSET, word);
          last = clock_systime_ticks();
        }
      else if (clock_systime_ticks() - last > MSEC2TICK(500))
        {
          syslog(LOG_ERR, "sai: fifo did not drain, cpu feed abandoned\n");
          return -ETIMEDOUT;
        }
    }

  return 0;
}


int stm32n6_sai_output_write(FAR const int16_t *samples, uint32_t count)
{
  struct stm32n6_sai_priv_s *priv = &g_sai1a_priv;
  uint32_t bytes;
  int      ret;

  if (!priv->initialized || samples == NULL || count == 0)
    {
      return -EINVAL;
    }

  bytes = count * sizeof(int16_t);

  /* One DMA block per call; the caller streams the file in chunks.  The
   * block length field is 16 bits.
   */

  if (bytes > 0xffff)
    {
      return -E2BIG;
    }

  /* The DMA reads this memory with the CPU's cache in the way otherwise. */

  up_clean_dcache((uintptr_t)samples, (uintptr_t)samples + bytes);

  if (g_sai_cpu_feed)
    {
      return stm32n6_sai_output_feed(samples, count);
    }

  ret = stm32n6_dma_start_m2p(SAI_TX_DMA_CHAN, (uint32_t)(uintptr_t)samples,
                              priv->base + SAI_DR_OFFSET, bytes,
                              SAI_TX_DMA_REQUEST, 1);
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_dma_wait(SAI_TX_DMA_CHAN, 1000);
  if (ret == -ETIMEDOUT)
    {
      /* The block has a clock and the codec is listening, so the audio can
       * go out without the channel.  Take the request line and the channel
       * out of the picture first, so nothing lands in the middle of the
       * transfer that follows, then feed the rest of the file from here.
       */

      stm32n6_dma_deinit(SAI_TX_DMA_CHAN);
      sai_putreg(priv, SAI_CR1_OFFSET,
                 sai_getreg(priv, SAI_CR1_OFFSET) & ~SAI_CR1_DMAEN);

      g_sai_cpu_feed = true;

      syslog(LOG_WARNING, "sai: no dma completion, feeding from the cpu "
             "instead\n");

      ret = stm32n6_sai_output_feed(samples, count);
    }

  return ret;
}


void stm32n6_sai_output_stop(void)
{
  struct stm32n6_sai_priv_s *priv = &g_sai1a_priv;

  if (!priv->initialized)
    {
      return;
    }

  stm32n6_dma_deinit(SAI_TX_DMA_CHAN);

  sai_putreg(priv, SAI_CR1_OFFSET,
             sai_getreg(priv, SAI_CR1_OFFSET) & ~SAI_CR1_SAIEN);

  priv->initialized = false;
}

void stm32n6_sai_output_dump(void)
{
  uint32_t cfgr1  = getreg32(STM32_RCC_PLL2CFGR1);
  uint32_t cfgr3  = getreg32(STM32_RCC_PLL2CFGR3);
  uint32_t ic7    = getreg32(STM32N6_RCC_IC7CFGR);
  uint32_t divm   = (cfgr1 >> 20) & 0x3f;
  uint32_t divn   = (cfgr1 >> 8) & 0xfff;
  uint32_t pdiv1  = (cfgr3 >> 27) & 0x7;
  uint32_t icint  = (ic7 >> RCC_ICCFGR_INT_SHIFT) & 0xff;
  uint32_t vco_khz;
  uint32_t ic_khz;
  uint32_t mckdiv;
  uint32_t us;
  uint32_t fflvl;
  int      i;

  if (divm == 0)
    {
      divm = 1;
    }

  if (pdiv1 == 0)
    {
      pdiv1 = 1;
    }

  vco_khz = (SAI_HSE_HZ / 1000 / divm) * divn;
  ic_khz  = vco_khz / pdiv1 / (icint + 1);

  mckdiv = (sai_getreg(&g_sai1a_priv, SAI_CR1_OFFSET) >> 20) & 0x3f;

  syslog(LOG_INFO, "sai: pll2 m=%lu n=%lu pdiv1=%lu -> vco %lu MHz; "
         "ic7 /%lu = %lu kHz; mckdiv %lu -> mclk %lu kHz, fs %lu Hz\n",
         (unsigned long)divm, (unsigned long)divn, (unsigned long)pdiv1,
         (unsigned long)(vco_khz / 1000), (unsigned long)(icint + 1),
         (unsigned long)ic_khz, (unsigned long)mckdiv,
         (unsigned long)(mckdiv != 0 ? ic_khz / mckdiv : 0),
         (unsigned long)(mckdiv != 0 ? ic_khz * 1000 / mckdiv / 256 : 0));

  syslog(LOG_INFO, "sai: cr1=%08lx cr2=%08lx frcr=%08lx slotr=%08lx "
         "sr=%08lx\n",
         (unsigned long)sai_getreg(&g_sai1a_priv, SAI_CR1_OFFSET),
         (unsigned long)sai_getreg(&g_sai1a_priv, SAI_CR2_OFFSET),
         (unsigned long)sai_getreg(&g_sai1a_priv, SAI_FRCR_OFFSET),
         (unsigned long)sai_getreg(&g_sai1a_priv, SAI_SLOTR_OFFSET),
         (unsigned long)sai_getreg(&g_sai1a_priv, SAI_SR_OFFSET));
  /* The divider gate lives in DIVENR; DIVENSR is the write-1-to-set alias and
   * reads back as zero, so the previous report of it said nothing at all.
   */

  syslog(LOG_INFO, "sai: ic7cfgr=%08lx divenr=%08lx (IC7EN=%lu)\n",
         (unsigned long)getreg32(STM32N6_RCC_IC7CFGR),
         (unsigned long)getreg32(STM32_RCC_DIVENR),
         (unsigned long)((getreg32(STM32_RCC_DIVENR) & RCC_DIVENR_IC7EN) ?
                         1 : 0));

  /* Clock probe.
   *
   * Push more words at the data register than the FIFO can hold, wait a
   * millisecond, and look at the level.  Eight words is under 200
   * microseconds of audio at 44.1 kHz, so a clocked block is empty by the
   * time this reads; an unclocked one is still full.  That one number says
   * whether the remaining problem is the clock or the DMA.
   */

  /* Clock probe: how long the block takes to empty its own FIFO.
   *
   * Eight words is 181 microseconds of audio at 44.1 kHz, so a clocked
   * block is empty by then and an unclocked one never empties at all.  One
   * number, and the range it has to be read over is five decades wide.
   */

  for (i = 0; i < 8; i++)
    {
      sai_putreg(&g_sai1a_priv, SAI_DR_OFFSET, 0);
    }

  for (us = 0; us < 2000; us += 50)
    {
      if (((sai_getreg(&g_sai1a_priv, SAI_SR_OFFSET) >> 16) & 0xf) == 0)
        {
          break;
        }

      up_udelay(50);
    }

  fflvl = (sai_getreg(&g_sai1a_priv, SAI_SR_OFFSET) >> 16) & 0xf;

  syslog(LOG_INFO, "sai: clock probe: fifo %lu of 8 left after about %lu us "
         "(44.1 kHz wants %lu)\n",
         (unsigned long)fflvl, (unsigned long)us,
         (unsigned long)(8 * 1000000UL / SAI_TX_RATE_HZ));
}

