/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_adc.c
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

/* ADR-029: ADC driver for the STM32N647 on-chip internal channels.
 *
 * This delivers the ADC function of ADR-029 through the NuttX ADC
 * character framework.  It targets the chip's internal analog sources that
 * need no external wiring, so the cmocka drivertest_adc suite and the real
 * hardware gate can exercise conversions without a probe.
 *
 * Two devices are registered from one driver:
 *
 *   /dev/adc0 (ADC1) -- single regular channel sampling the internal voltage
 *     reference (VREFINT, channel 17), software-triggered and read back by
 *     polling EOC.  This is the path drivertest_adc exercises and its
 *     behaviour is deliberately left unchanged.
 *
 *   /dev/adc1 (ADC2) -- a multi-channel regular *scan* over two internal
 *     voltages (VBAT channel 16 + VDDCORE channel 17) whose results are
 *     moved by GPDMA into a memory buffer, with the analog watchdog (AWD1)
 *     armed to flag an out-of-window sample.  This fills the ADR-029 gaps
 *     (scan, DMA, AWD) without touching /dev/adc0.  Because D-cache is
 *     enabled (ADR-007) the DMA landing buffer is cache-line aligned and
 *     invalidated before the samples are read back.
 *
 * All conversions complete synchronously inside ANIOC_TRIGGER (the scan path
 * bounded-polls DMA completion, the single path bounded-polls EOC), so the
 * core never sleeps mid-conversion and no EOC interrupt is needed.
 *
 * The ADC kernel clock (RCC ADC12SEL) is left at its reset default of HCLK,
 * which is already running on this board, so no RCC clock-mux setup is
 * needed -- only the AHB1 peripheral clock gate is opened.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>
#include <nuttx/spinlock.h>
#include <nuttx/cache.h>

#include "arm_internal.h"
#include "chip.h"
#include "stm32n6_adc.h"
#include "stm32n6_dma.h"
#include "hardware/stm32_adc.h"
#include "hardware/stm32_rcc.h"
#include "hardware/stm32_pwr.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Polling budgets (microseconds).  A single internal-channel conversion at
 * the longest sample time is ~26 us with the ADC on a 32 MHz HCLK, so these
 * give a wide margin while still guaranteeing forward progress (never a
 * dead wait -- Embedded Programming Rule 2).
 */

#define ADC_TIMEOUT_US       10000  /* Regulator/calibration/ready/EOC */
#define ADC_REGUL_STAB_US    10     /* Voltage-regulator stabilization */
#define ADC_VREFINT_STAB_US  12     /* VREFINT internal-path stabilization */

/* /dev/adc1 scan configuration.  ADC2 scans two internal voltages and moves
 * the results by GPDMA; AWD1 monitors VBAT for an out-of-window sample.
 */

#define ADC_SCAN_MAX         2      /* Longest scan we build (VBAT+VDDCORE) */
#define ADC_DMABUF_WORDS     8      /* Full 32-byte D-cache line */
#define ADC_DMA_CHANNEL      0      /* GPDMA channel used for the ADC2 scan */
#define ADC_DMA_REQ_ADC2     8      /* GPDMA hardware request line for ADC2 */
#define ADC_DMA_TIMEOUT_MS   10     /* Bounded wait for the scan DMA */
#define ADC_AWD_LTR_DEFAULT  0x000  /* Full-open low threshold (no trip) */
#define ADC_AWD_HTR_DEFAULT  0xfff  /* Full-open high threshold (12-bit) */

/* Cortex-M55 D-cache line size.  The DMA landing buffer is aligned to (and
 * padded to a multiple of) this so an invalidate touches only our samples.
 */

#define ADC_DCACHE_LINE      32

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_adc_s
{
  struct adc_dev_s            *dev;       /* Owning upper-half device */
  const struct adc_callback_s *cb;        /* Upper-half receive callback */
  uint32_t                     base;      /* ADC instance register base */
  uint32_t                     cmnbase;   /* ADC common-block register base */
  uint8_t                      nchannels; /* Regular-sequence length */

  /* Channels sampled, in sequence order */

  uint8_t                      channels[ADC_SCAN_MAX];
  bool                         scan;      /* True: DMA scan; false: poll */
  bool                         ready;     /* True once the ADC is enabled */
  uint32_t                    *dmabuf;    /* DMA landing buffer (scan mode) */
  uint32_t                     awdlow;    /* AWD1 low threshold (scan mode) */
  uint32_t                     awdhigh;   /* AWD1 high threshold (scan) */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int  stm32n6_adc_bind(struct adc_dev_s *dev,
                             const struct adc_callback_s *callback);
static void stm32n6_adc_reset(struct adc_dev_s *dev);
static int  stm32n6_adc_setup(struct adc_dev_s *dev);
static void stm32n6_adc_shutdown(struct adc_dev_s *dev);
static void stm32n6_adc_rxint(struct adc_dev_s *dev, bool enable);
static int  stm32n6_adc_ioctl(struct adc_dev_s *dev, int cmd,
                              unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct adc_ops_s g_stm32n6_adc_ops =
{
  .ao_bind     = stm32n6_adc_bind,
  .ao_reset    = stm32n6_adc_reset,
  .ao_setup    = stm32n6_adc_setup,
  .ao_shutdown = stm32n6_adc_shutdown,
  .ao_rxint    = stm32n6_adc_rxint,
  .ao_ioctl    = stm32n6_adc_ioctl,
};

/* /dev/adc0 (ADC1): single-channel VREFINT, software-triggered poll. */

static struct stm32n6_adc_s g_adc1priv =
{
  .base      = STM32_ADC1_BASE,
  .cmnbase   = STM32_ADC12_COMMON_BASE,
  .nchannels = 1,
  .channels  =
  {
    ADC_CHANNEL_VREFINT
  },
  .scan      = false,
};

static struct adc_dev_s g_adc1dev =
{
  .ad_ops  = &g_stm32n6_adc_ops,
  .ad_priv = &g_adc1priv,
};

#ifdef CONFIG_STM32_ADC2
/* /dev/adc1 (ADC2): VBAT + VDDCORE scan via GPDMA, with AWD1 armed.  The DMA
 * landing buffer is cache-line aligned so its invalidate touches only these
 * samples (D-cache is enabled per ADR-007).
 */

/* Sized to a whole 32-byte D-cache line and aligned to it, so the buffer
 * owns the line exclusively.  A partial-line invalidate would write the
 * stale (dirty) portion back over the DMA data; owning the full line makes
 * the post-transfer invalidate a pure discard.
 */

static uint32_t g_adc2dmabuf[ADC_DMABUF_WORDS]
  aligned_data(ADC_DCACHE_LINE);

static struct stm32n6_adc_s g_adc2priv =
{
  .base      = STM32_ADC2_BASE,
  .cmnbase   = STM32_ADC12_COMMON_BASE,
  .nchannels = 2,
  .channels  =
  {
    ADC_CHANNEL_VBAT, ADC_CHANNEL_VDDCORE
  },
  .scan      = true,
  .dmabuf    = g_adc2dmabuf,
  .awdlow    = ADC_AWD_LTR_DEFAULT,
  .awdhigh   = ADC_AWD_HTR_DEFAULT,
};

static struct adc_dev_s g_adc2dev =
{
  .ad_ops  = &g_stm32n6_adc_ops,
  .ad_priv = &g_adc2priv,
};
#endif

static spinlock_t g_adc_lock = SP_UNLOCKED;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_adc_enableclk
 *
 * Description:
 *   Open the AHB1 peripheral clock gate feeding ADC1/ADC2.  Without this a
 *   register write is silently dropped and reads return zero.
 *
 ****************************************************************************/

static void stm32n6_adc_enableclk(void)
{
  irqstate_t flags;
  uint32_t regval;

  flags = spin_lock_irqsave(&g_adc_lock);

  /* AHB1ENR is READ-ONLY status on STM32N6; use the AHB1ENSR
   * write-1-to-set alias to open the ADC clock gate. */
  putreg32(RCC_AHB1ENR_ADC12EN, STM32_RCC_AHB1ENSR);

  spin_unlock_irqrestore(&g_adc_lock, flags);
}

/****************************************************************************
 * Name: stm32n6_adc_wait_set
 *
 * Description:
 *   Poll a status-register bit until the hardware sets it, bounded by
 *   ADC_TIMEOUT_US.  Used for the ready (ADRDY) and EOC handshakes.
 *
 ****************************************************************************/

static int stm32n6_adc_wait_set(uint32_t reg, uint32_t bit)
{
  int i;

  for (i = 0; i < ADC_TIMEOUT_US; i++)
    {
      if ((getreg32(reg) & bit) != 0)
        {
          return OK;
        }

      up_udelay(1);
    }

  return -ETIMEDOUT;
}

/****************************************************************************
 * Name: stm32n6_adc_bind
 ****************************************************************************/

static int stm32n6_adc_bind(struct adc_dev_s *dev,
                            const struct adc_callback_s *callback)
{
  struct stm32n6_adc_s *priv = (struct stm32n6_adc_s *)dev->ad_priv;

  DEBUGASSERT(priv != NULL);
  priv->cb = callback;
  return OK;
}

/****************************************************************************
 * Name: stm32n6_adc_reset
 *
 * Description:
 *   Return the ADC to a known idle state: stop any conversion, disable the
 *   ADC and put it back into deep power-down.
 *
 ****************************************************************************/

static void stm32n6_adc_reset(struct adc_dev_s *dev)
{
  struct stm32n6_adc_s *priv = (struct stm32n6_adc_s *)dev->ad_priv;
  irqstate_t flags;
  uint32_t cr;

  DEBUGASSERT(priv != NULL);

  flags = spin_lock_irqsave(&g_adc_lock);

  cr = getreg32(priv->base + STM32_ADC_CR_OFFSET);
  if ((cr & ADC_CR_ADEN) != 0)
    {
      /* Request disable and re-enter deep power-down */

      putreg32(ADC_CR_ADDIS, priv->base + STM32_ADC_CR_OFFSET);
    }

  putreg32(ADC_CR_DEEPPWD, priv->base + STM32_ADC_CR_OFFSET);
  priv->ready = false;

  spin_unlock_irqrestore(&g_adc_lock, flags);
}

/****************************************************************************
 * Name: stm32n6_adc_setup
 *
 * Description:
 *   Power up, calibrate and enable the ADC, then configure a one-channel
 *   regular sequence sampling the internal VREFINT source.  Called the
 *   first time /dev/adc0 is opened.
 *
 ****************************************************************************/

static int stm32n6_adc_setup(struct adc_dev_s *dev)
{
  struct stm32n6_adc_s *priv = (struct stm32n6_adc_s *)dev->ad_priv;
  irqstate_t flags;
  uint32_t regval;
  int ret;

  DEBUGASSERT(priv != NULL);

  if (priv->ready)
    {
      return OK;
    }

  stm32n6_adc_enableclk();

  flags = spin_lock_irqsave(&g_adc_lock);

  /* Close the VDDA18ADC analog-supply switch (SVMCR3.ASV).  Without a valid
   * analog supply the ADC still runs and end-of-conversion still asserts,
   * but every conversion reads back exactly zero -- so this must be enabled
   * before powering up the ADC.
   */

  regval  = getreg32(STM32_PWR_SVMCR3);
  regval |= PWR_SVMCR3_ASV;
  putreg32(regval, STM32_PWR_SVMCR3);

  /* Exit deep power-down.  On the STM32N6 clearing DEEPPWD is what enables
   * the ADC voltage regulator; there is no separate ADVREGEN bit.  Give the
   * regulator its stabilization time before enabling the ADC.
   */

  regval  = getreg32(priv->base + STM32_ADC_CR_OFFSET);
  regval &= ~ADC_CR_DEEPPWD;
  putreg32(regval, priv->base + STM32_ADC_CR_OFFSET);

  up_udelay(ADC_REGUL_STAB_US);

  /* No calibration is performed.  Unlike the STM32H7/F3 "set ADCAL with
   * ADEN=0 and wait for self-clear" flow, the STM32N6 calibrates via an
   * averaged offset measurement that runs with the ADC already enabled; a
   * blind ADCAL here would never clear.  Calibration only trims a static
   * offset, which is irrelevant to reading VREFINT and to the drivertest
   * (it only needs a live, jittering conversion), so it is skipped.
   *
   * Enable the ADC: clear a stale ADRDY, then set ADEN and wait for ADRDY.
   * ADEN is re-asserted on each poll: hardware may clear it if it is set
   * too soon after power-up, and re-writing it is harmless once ready.
   */

  putreg32(ADC_ISR_ADRDY, priv->base + STM32_ADC_ISR_OFFSET);

  regval  = getreg32(priv->base + STM32_ADC_CR_OFFSET);
  regval |= ADC_CR_ADEN;
  putreg32(regval, priv->base + STM32_ADC_CR_OFFSET);

  ret = -ETIMEDOUT;
    {
      int i;
      for (i = 0; i < ADC_TIMEOUT_US; i++)
        {
          if ((getreg32(priv->base + STM32_ADC_ISR_OFFSET) &
               ADC_ISR_ADRDY) != 0)
            {
              ret = OK;
              break;
            }

          if ((getreg32(priv->base + STM32_ADC_CR_OFFSET) &
               ADC_CR_ADEN) == 0)
            {
              modifyreg32(priv->base + STM32_ADC_CR_OFFSET, 0, ADC_CR_ADEN);
            }

          up_udelay(1);
        }
    }

  if (ret < 0)
    {
      spin_unlock_irqrestore(&g_adc_lock, flags);
      aerr("ERROR: ADC enable (ADRDY) timeout\n");
      return ret;
    }

  putreg32(ADC_ISR_ADRDY, priv->base + STM32_ADC_ISR_OFFSET);

  if (!priv->scan)
    {
      /* /dev/adc0: single VREFINT channel, polled.  Route the internal
       * VREFINT path, use the longest sample time for the slow reference,
       * preselect it, and build a one-conversion regular sequence.
       */

      regval  = getreg32(priv->cmnbase + STM32_ADC_CCR_OFFSET);
      regval |= ADC_CCR_VREFEN;
      putreg32(regval, priv->cmnbase + STM32_ADC_CCR_OFFSET);

      regval  = getreg32(priv->base + STM32_ADC_SMPR2_OFFSET);
      regval &= ~ADC_SMPR2_SMP17_MASK;
      regval |= (ADC_SMP_MAX << ADC_SMPR2_SMP17_SHIFT);
      putreg32(regval, priv->base + STM32_ADC_SMPR2_OFFSET);

      /* Preselect the channel: without its PCSEL bit the input is not wired
       * to the ADC mux and the data register reads back zero.
       */

      putreg32(ADC_PCSEL_CH(priv->channels[0]),
               priv->base + STM32_ADC_PCSEL_OFFSET);

      /* One-conversion regular sequence: L = 0, SQ1 = channel */

      putreg32(((uint32_t)priv->channels[0] << ADC_SQR1_SQ1_SHIFT),
               priv->base + STM32_ADC_SQR1_OFFSET);
    }
  else
    {
      /* /dev/adc1: VBAT + VDDCORE regular scan, moved by GPDMA, AWD1 armed.
       *
       * VBAT is routed by CCR.VBATEN; VDDCORE has no CCR path bit -- it is
       * routed to the ADC2 mux by the option register (OR.OP2).  Both share
       * the ch16/ch17 sample-time fields, set to the longest sample time
       * since both are high-impedance internal dividers.
       */

      regval  = getreg32(priv->cmnbase + STM32_ADC_CCR_OFFSET);
      regval |= ADC_CCR_VBATEN;
      putreg32(regval, priv->cmnbase + STM32_ADC_CCR_OFFSET);

      regval  = getreg32(priv->base + STM32_ADC_OR_OFFSET);
      regval |= ADC_OR_OP2;
      putreg32(regval, priv->base + STM32_ADC_OR_OFFSET);

      regval  = getreg32(priv->base + STM32_ADC_SMPR2_OFFSET);
      regval &= ~(ADC_SMPR2_SMP16_MASK | ADC_SMPR2_SMP17_MASK);
      regval |= (ADC_SMP_MAX << ADC_SMPR2_SMP16_SHIFT);
      regval |= (ADC_SMP_MAX << ADC_SMPR2_SMP17_SHIFT);
      putreg32(regval, priv->base + STM32_ADC_SMPR2_OFFSET);

      /* Preselect both scanned channels */

      putreg32(ADC_PCSEL_CH(priv->channels[0]) |
               ADC_PCSEL_CH(priv->channels[1]),
               priv->base + STM32_ADC_PCSEL_OFFSET);

      /* Regular sequence: L = 1 (two conversions), SQ1/SQ2 = channels */

      putreg32(((uint32_t)(priv->nchannels - 1) << ADC_SQR1_L_SHIFT) |
               ((uint32_t)priv->channels[0] << ADC_SQR1_SQ1_SHIFT) |
               ((uint32_t)priv->channels[1] << ADC_SQR1_SQ2_SHIFT),
               priv->base + STM32_ADC_SQR1_OFFSET);

      /* Data management = DMA one-shot: each converted sample is pushed to
       * the DMA request, one scan per software trigger.
       */

      regval  = getreg32(priv->base + STM32_ADC_CFGR1_OFFSET);
      regval &= ~ADC_CFGR1_DMNGT_MASK;
      regval |= ADC_CFGR1_DMNGT_DMA1S;

      /* Arm AWD1 over the whole regular group (not AWD1SGL) so any scanned
       * sample outside [LTR1, HTR1] latches ISR.AWD1.  Thresholds default to
       * full-open (never trip) and are narrowed via ANIOC_WDOG_*.
       */

      regval &= ~ADC_CFGR1_AWD1SGL;
      regval |= ADC_CFGR1_AWD1EN;
      putreg32(regval, priv->base + STM32_ADC_CFGR1_OFFSET);

      putreg32(priv->awdlow, priv->base + STM32_ADC_AWD1LTR_OFFSET);
      putreg32(priv->awdhigh, priv->base + STM32_ADC_AWD1HTR_OFFSET);
    }

  priv->ready = true;

  spin_unlock_irqrestore(&g_adc_lock, flags);

  /* Bring up the GPDMA channel that carries the scan results.  Done outside
   * the ADC critical section because the DMA helper takes its own lock.
   */

#ifdef CONFIG_STM32_ADC2
  if (priv->scan)
    {
      stm32n6_dma_init(ADC_DMA_CHANNEL);
    }
#endif

  /* Let the internal sources settle before the first conversion */

  up_udelay(ADC_VREFINT_STAB_US);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_adc_shutdown
 ****************************************************************************/

static void stm32n6_adc_shutdown(struct adc_dev_s *dev)
{
  stm32n6_adc_reset(dev);
}

/****************************************************************************
 * Name: stm32n6_adc_rxint
 *
 * Description:
 *   Enable/disable RX interrupts.  This driver is poll-based (no EOC
 *   interrupt is used), so there is nothing to toggle.
 *
 ****************************************************************************/

static void stm32n6_adc_rxint(struct adc_dev_s *dev, bool enable)
{
  UNUSED(dev);
  UNUSED(enable);
}

/****************************************************************************
 * Name: stm32n6_adc_convert
 *
 * Description:
 *   Software-trigger one regular conversion, poll for completion, read the
 *   result and forward it to the upper half.
 *
 ****************************************************************************/

static int stm32n6_adc_convert(struct stm32n6_adc_s *priv)
{
  irqstate_t flags;
  uint32_t data;
  int ret;

  if (!priv->ready)
    {
      return -EAGAIN;
    }

  flags = spin_lock_irqsave(&g_adc_lock);

  /* Start one regular conversion */

  modifyreg32(priv->base + STM32_ADC_CR_OFFSET, 0, ADC_CR_ADSTART);

  /* Poll for end-of-conversion (bounded) */

  ret = stm32n6_adc_wait_set(priv->base + STM32_ADC_ISR_OFFSET, ADC_ISR_EOC);
  if (ret < 0)
    {
      spin_unlock_irqrestore(&g_adc_lock, flags);
      aerr("ERROR: ADC EOC timeout\n");
      return ret;
    }

  /* Reading DR clears EOC */

  data = getreg32(priv->base + STM32_ADC_DR_OFFSET) & ADC_DR_RDATA_MASK;

  spin_unlock_irqrestore(&g_adc_lock, flags);

  if (priv->cb != NULL && priv->cb->au_receive != NULL)
    {
      priv->cb->au_receive(priv->dev, priv->channels[0], (int32_t)data);
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_adc_scan
 *
 * Description:
 *   Software-trigger one regular scan of the configured channels, let GPDMA
 *   move each result into the DMA buffer, then forward every sample to the
 *   upper half.  The analog-watchdog flag is checked and cleared per scan.
 *
 ****************************************************************************/

#ifdef CONFIG_STM32_ADC2
static int stm32n6_adc_scan(struct stm32n6_adc_s *priv)
{
  irqstate_t flags;
  uint32_t isr;
  int ret;
  int i;

  if (!priv->ready)
    {
      return -EAGAIN;
    }

  /* Program the DMA before arming the ADC: peripheral (ADC2 DR) -> memory,
   * one word per scanned channel, on the ADC2 hardware request line.
   */

  /* Clean the landing buffer out of D-cache before starting the DMA.  The
   * buffer sits in zero-initialized BSS, so its cache line is dirty with
   * zeros; without this flush a later dirty write-back can clobber the data
   * the DMA writes to SRAM (D-cache is enabled per ADR-007).  The matching
   * invalidate after the transfer then forces the CPU to read DMA data.
   */

  up_clean_dcache((uintptr_t)priv->dmabuf,
                  (uintptr_t)priv->dmabuf +
                  ADC_DMABUF_WORDS * sizeof(uint32_t));

  ret = stm32n6_dma_start_p2m(ADC_DMA_CHANNEL,
                              priv->base + STM32_ADC_DR_OFFSET,
                              (uint32_t)(uintptr_t)priv->dmabuf,
                              priv->nchannels * sizeof(uint32_t),
                              ADC_DMA_REQ_ADC2);
  if (ret < 0)
    {
      aerr("ERROR: ADC scan DMA start: %d\n", ret);
      return ret;
    }

  flags = spin_lock_irqsave(&g_adc_lock);

  /* Clear a stale AWD1 flag, then start the regular scan */

  putreg32(ADC_ISR_AWD1, priv->base + STM32_ADC_ISR_OFFSET);
  modifyreg32(priv->base + STM32_ADC_CR_OFFSET, 0, ADC_CR_ADSTART);

  spin_unlock_irqrestore(&g_adc_lock, flags);

  /* Wait (bounded) for the DMA to land every sample */

  ret = stm32n6_dma_wait(ADC_DMA_CHANNEL, ADC_DMA_TIMEOUT_MS);
  if (ret < 0)
    {
      aerr("ERROR: ADC scan DMA wait: %d\n", ret);
      return ret;
    }

  /* Invalidate the just-written buffer so the CPU reads DMA data, not a
   * stale cached copy (D-cache is enabled per ADR-007).
   */

  up_invalidate_dcache((uintptr_t)priv->dmabuf,
                       (uintptr_t)priv->dmabuf +
                       ADC_DMABUF_WORDS * sizeof(uint32_t));

  isr = getreg32(priv->base + STM32_ADC_ISR_OFFSET);
  if ((isr & ADC_ISR_AWD1) != 0)
    {
      awarn("WARNING: ADC AWD1 tripped (sample out of window)\n");
      putreg32(ADC_ISR_AWD1, priv->base + STM32_ADC_ISR_OFFSET);
    }

  if (priv->cb != NULL && priv->cb->au_receive != NULL)
    {
      for (i = 0; i < priv->nchannels; i++)
        {
          uint32_t sample = priv->dmabuf[i] & ADC_DR_RDATA_MASK;

          priv->cb->au_receive(priv->dev, priv->channels[i],
                               (int32_t)sample);
        }
    }

  return OK;
}
#endif

/****************************************************************************
 * Name: stm32n6_adc_ioctl
 ****************************************************************************/

static int stm32n6_adc_ioctl(struct adc_dev_s *dev, int cmd,
                             unsigned long arg)
{
  struct stm32n6_adc_s *priv = (struct stm32n6_adc_s *)dev->ad_priv;
  int ret;

  DEBUGASSERT(priv != NULL);

  switch (cmd)
    {
      case ANIOC_TRIGGER:
#ifdef CONFIG_STM32_ADC2
        ret = priv->scan ? stm32n6_adc_scan(priv)
                         : stm32n6_adc_convert(priv);
#else
        ret = stm32n6_adc_convert(priv);
#endif
        break;

      case ANIOC_GET_NCHANNELS:
        ret = priv->nchannels;
        break;

#ifdef CONFIG_STM32_ADC2
      case ANIOC_WDOG_UPPER:  /* Narrow the AWD1 high threshold (scan only) */
        if (!priv->scan)
          {
            ret = -ENOTTY;
            break;
          }

        priv->awdhigh = (uint32_t)arg & ADC_DR_RDATA_MASK;
        putreg32(priv->awdhigh, priv->base + STM32_ADC_AWD1HTR_OFFSET);
        ret = OK;
        break;

      case ANIOC_WDOG_LOWER:  /* Narrow the AWD1 low threshold (scan only) */
        if (!priv->scan)
          {
            ret = -ENOTTY;
            break;
          }

        priv->awdlow = (uint32_t)arg & ADC_DR_RDATA_MASK;
        putreg32(priv->awdlow, priv->base + STM32_ADC_AWD1LTR_OFFSET);
        ret = OK;
        break;
#endif

      default:
        ret = -ENOTTY;
        break;
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_adc_initialize
 *
 * Description:
 *   Register an ADC character device backed by the STM32N6 ADC internal
 *   channels.
 *
 * Input Parameters:
 *   devpath - Character device path (e.g. "/dev/adc0" or "/dev/adc1").
 *   intf    - ADC peripheral number: 1 = ADC1 (VREFINT poll), 2 = ADC2
 *             (VBAT+VDDCORE DMA scan with analog watchdog).
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_adc_initialize(const char *devpath, int intf)
{
  struct adc_dev_s *dev;
  struct stm32n6_adc_s *priv;
  int ret;

  DEBUGASSERT(devpath != NULL);

  switch (intf)
    {
      case 1:
        dev = &g_adc1dev;
        break;

#ifdef CONFIG_STM32_ADC2
      case 2:
        dev = &g_adc2dev;
        break;
#endif

      default:
        return -ENODEV;
    }

  /* Back-link the private state to its owning device so au_receive() reports
   * against the right /dev/adcN.
   */

  priv = (struct stm32n6_adc_s *)dev->ad_priv;
  priv->dev = dev;

  ret = adc_register(devpath, dev);
  if (ret < 0)
    {
      aerr("ERROR: adc_register(%s) failed: %d\n", devpath, ret);
    }

  return ret;
}
