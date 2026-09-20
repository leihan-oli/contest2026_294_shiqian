/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_dcmipp.c
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
 * STM32N6 DCMIPP (Digital Camera Interface Pixel Pipeline) driver.
 *
 * Register sequences ported from ST HAL_DCMIPP (STM32Cube_FW_N6
 * stm32n6xx_hal_dcmipp.c) for the ALIENTEK STM32N647 board camera:
 *   CSI-2: 2 data lanes, PHY BT_1600 (HSFR=0x0D / OSC=295 / CCFR=0x28,
 *          matches bare-metal PFCR=0x10D28).
 *   PIPE1: RAW10 -> RGB565 (hardware ISP demosaic), 800x480 downsize,
 *          continuous capture directly into the LTDC framebuffer.
 *
 *   Capture flow follows the bare-metal diag9f reference: sensor
 *   configure + streaming (0x3000=0) + INCK 37 MHz PLL FIRST, then
 *   VC0 + PIPE1 are armed LAST.
 *
 * The DCMIPP writes the framebuffer as an AXI bus master (like DMA2D);
 * the RIMC master attributes are set to SEC|PRIV so it can reach the
 * secure framebuffer SRAM (validated with the RISAF IAR method).
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/clock.h>
#include <debug.h>
#include <errno.h>

#include <arch/irq.h>

#include "arm_internal.h"
#include "nvic.h"
#include "chip.h"
#include "hardware/stm32_dcmipp.h"
#include "hardware/stm32_rcc.h"
#include "hardware/stm32_dma2d.h"
#include "stm32n6_dcmipp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* DCMIPP kernel clock: PLL1 (1200 MHz VCO, left by FSBL) / IC17 divider 4
 * = 300 MHz.  Matches the bare-metal HAL_DCMIPP_MspInit (appli.hex
 * IC17CFGR = 0x10030000, divider 4).  The 600 MHz experiment (divider 2)
 * did not fix the stall and increased PIPE1 overruns (108 vs 71), so the
 * clock is left at the reference value.
 */

#define DCMIPP_IC17_DIVIDER    (4)

/* CSIPHY config clock (ck_ker_csiphy = ic18_ck, RM0486 Table 73).
 * IC18 = PLL1/60 = 20 MHz, the exact bare-metal value (HAL_DCMIPP_MspInit).
 */

#define DCMIPP_IC18_DIVIDER    (60)   /* PLL1/60 = 20 MHz (bare-metal exact) */

/* CSI D-PHY frequency-range / DLL oscillator / config-clock constants.
 * Values match the bare-metal reference exactly (PFCR=0x10D28):
 * HSFR=0x0D (BT_1600 hsfreqrange), OSC=295 (DLL oscillator target),
 * CCFR=0x28.  Do not change without re-validating against diag9f.
 */

#define DCMIPP_PHY_HSFR   (0x0du)   /* HAL_CSI_BT_1600 hsfreqrange */
#define DCMIPP_PHY_OSC    (295u)    /* HAL_CSI_BT_1600 DLL osc target */
#define DCMIPP_PHY_CCFR   (0x28u)   /* bare-metal PFCR=0x10D28 CCFR */

/* DCMIPP stop timeout (ms) */

#define DCMIPP_STOP_TIMEOUT    (100)

/* CSI-2 error interrupt enables (IER0 / IER1) -- all error cases */

#define DCMIPP_CSI_IER0_ENABLE \
  (CSI_IER0_CCFIFOFIE | CSI_IER0_SYNCERRIE | CSI_IER0_SPKTERRIE | \
   CSI_IER0_IDERRIE | CSI_IER0_SPKTIE | \
   CSI_IER0_SOF0IE | CSI_IER0_EOF0IE)

#define DCMIPP_CSI_IER1_ENABLE \
  (CSI_IER1_ESOTDL1IE | CSI_IER1_ESOTSYNCDL1IE | CSI_IER1_EESCDL1IE | \
   CSI_IER1_ESYNCESCDL1IE | CSI_IER1_ECTRLDL1IE | \
   CSI_IER1_ESOTDL0IE | CSI_IER1_ESOTSYNCDL0IE | CSI_IER1_EESCDL0IE | \
   CSI_IER1_ESYNCESCDL0IE | CSI_IER1_ECTRLDL0IE)

/* PIPE1 interrupt enables used on capture start */

#define DCMIPP_PIPE1_IT_ENABLE \
  (DCMIPP_CMIER_P1FRAMEIE | DCMIPP_CMIER_P1VSYNCIE | \
   DCMIPP_CMIER_P1OVRIE | DCMIPP_CMIER_ATXERRIE)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct dcmipp_priv_s
{
  volatile uint32_t frame_count;      /* PIPE1 completed frames */
  volatile uint32_t error_code;       /* sticky error flags */
  volatile uint32_t overrun_count;    /* PIPE1 overrun events */
  volatile int      csi_err_count;    /* CSI error rate-limit */
  volatile uint32_t csi_err_sr0;      /* CSI SR0 latched at 1st error */
  volatile uint32_t csi_err_sr1;      /* CSI SR1 latched at 1st error */
  dcmipp_frame_cb_t frame_cb;         /* PIPE1 frame event callback */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct dcmipp_priv_s g_dcmipp;

/* Runtime-selectable CSI D-PHY RX parameter mode (see
 * stm32n6_dcmipp_set_rx_mode):
 *   0 = baseline (DLL osc 295, deskew 0x38)  - matches bare-metal
 *   1 = deskew polarity reg 0x08 = 0x00
 *   2 = deskew polarity reg 0x08 = 0x3f
 *   3 = DLL osc target 285
 *   4 = DLL osc target 305
 */

static int g_dcmipp_rx_mode = 0;


/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dcmipp_clock_config
 *
 * Description:
 *   Enable DCMIPP + CSI-2 clocks and configure their kernel clocks:
 *   IC17 = PLL1 / 4  = 300 MHz (DCMIPP, matches bare-metal MspInit)
 *   IC18 = PLL1 / 44 = 27.27 MHz (CSIPHY config clock, matches CCFR=0x28)
 *   CCIPR1.DCMIPPSEL = IC17
 *
 ****************************************************************************/

static void dcmipp_clock_config(void)
{
  uint32_t regval;

  /* IC17: source = PLL1 (SEL = 0b01), divider = 4 (INT = 3) = 300 MHz */

  regval = ((DCMIPP_IC17_DIVIDER - 1) << RCC_IC17CFGR_IC17INT_SHIFT) |
           RCC_IC17CFGR_IC17SEL_PLL1;
  putreg32(regval, STM32_RCC_IC17CFGR);

  /* IC18: source = PLL1 (SEL = 0b01), divider = 60 (INT = 59)
   * = 20 MHz, exact bare-metal value (appli HAL_DCMIPP_MspInit).
   */

  regval = ((DCMIPP_IC18_DIVIDER - 1) << RCC_IC18CFGR_IC18INT_SHIFT) |
           RCC_IC18CFGR_IC18SEL_PLL1;
  putreg32(regval, STM32_RCC_IC18CFGR);

  /* Enable the IC17 / IC18 dividers (write-1-to-set alias) */

  putreg32(RCC_DIVENR_IC17EN | RCC_DIVENR_IC18EN, STM32_RCC_DIVENSR);

  /* (2026-08-25) Wait for the freshly-enabled IC17/IC18 dividers to
   * stabilise before the CSI D-PHY is configured.  The D-PHY DLL uses
   * the CSI reference clock (IC18); if the PHY is programmed while the
   * reference clock is still settling, the deskew/DLL test-register
   * writes do not take and the link never samples correctly (persistent
   * CRC errors).  On bare-metal IC18 was configured by the FSBL at boot
   * and stable for a long time before the PHY was touched. */

  up_mdelay(50);

  /* Select IC17 as the DCMIPP kernel clock source (CCIPR1.DCMIPPSEL=0b10),
   * matching bare-metal HAL_RCCEx_PeriphCLKConfig.
   */

  modifyreg32(STM32_RCC_CCIPR1, RCC_CCIPR1_DCMIPPSEL_MASK,
              RCC_CCIPR1_DCMIPPSEL_IC17);

  /* Enable the APB5 DCMIPP and CSI clocks (write-1-to-set alias) */

  putreg32(RCC_APB5ENR_DCMIPPEN | RCC_APB5ENR_CSIEN, STM32_RCC_APB5ENSR);

  /* ROOT CAUSE (2026-08-23): after 'cam start' returns, the IDLE task runs
   * WFI (up_idle) and the CPU enters Sleep.  RCC_APB5LPENR resets to 0, so
   * during Sleep the DCMIPP/CSI APB5 clocks are gated (EN AND LPEN = 0) and
   * the camera pipeline stops (CCFIFOFF, no frames) even though everything
   * is configured correctly -- same class of bug as the LTDC black-screen
   * that was fixed with LTDCLPEN.  Keep DCMIPP/CSI clocked in Sleep via the
   * APB5LPENSR set alias. */

  putreg32(RCC_APB5LPENR_DCMIPPLPEN | RCC_APB5LPENR_CSILPEN,
           STM32_RCC_APB5LPENSR);

  /* Take DCMIPP and CSI out of reset */

  putreg32(RCC_APB5RSTR_DCMIPPRST | RCC_APB5RSTR_CSIRST,
           STM32_RCC_APB5RSTR);
  putreg32(0, STM32_RCC_APB5RSTR);

  /* DIAG (2026-08-25): read back the clock enables to confirm IC17/IC18
   * really enabled and the APB5 DCMIPP/CSI clocks are on (a missing
   * enable would leave the CSI/PHY without a reference clock and the
   * D-PHY could never lock -> CRC errors). */

  _info("dcmipp: RCC IC17CFGR=0x%08lx IC18CFGR=0x%08lx DIVENR=0x%08lx\n",
        (unsigned long)getreg32(STM32_RCC_IC17CFGR),
        (unsigned long)getreg32(STM32_RCC_IC18CFGR),
        (unsigned long)getreg32(STM32_RCC_DIVENR));
  _info("dcmipp: RCC CCIPR1=0x%08lx APB5ENR=0x%08lx APB5LPENR=0x%08lx\n",
        (unsigned long)getreg32(STM32_RCC_CCIPR1),
        (unsigned long)getreg32(STM32_RCC_APB5ENR),
        (unsigned long)getreg32(STM32_RCC_APB5LPENR));
  _info("dcmipp: CLK PLL1CFGR1=0x%08lx PLL1CFGR3=0x%08lx "
        "CFGR2=0x%08lx\n",
        (unsigned long)getreg32(STM32_RCC_PLL1CFGR1),
        (unsigned long)getreg32(STM32_RCC_PLL1CFGR3),
        (unsigned long)getreg32(STM32_RCC_CFGR2));
  _info("dcmipp: CLK IC17CFGR=0x%08lx IC18CFGR=0x%08lx\n",
        (unsigned long)getreg32(STM32_RCC_IC17CFGR),
        (unsigned long)getreg32(STM32_RCC_IC18CFGR));
}

/****************************************************************************
 * Name: dcmipp_rif_config
 *
 * Description:
 *   Configure the DCMIPP RIMC master attributes: CID1, secure, privileged.
 *   Without this the DCMIPP presents CID0/NS transactions and its writes
 *   to the secure framebuffer SRAM are rejected by the RISAF.
 *
 ****************************************************************************/

static void dcmipp_rif_config(void)
{
  /* Enable the RIFSC clock (AHB3) before touching RIMC registers */

  putreg32(RCC_AHB3ENR_RIFSCEN, STM32_RCC_AHB3ENSR);

  /* DCMIPP is RIMC master index 9.  SEC|PRIV + CID1 matches the
   * bare-metal SystemIsolation_Config.
   */

  putreg32(RIFSC_RIMC_ATTR_MCID(1) | RIFSC_RIMC_ATTR_MSEC |
           RIFSC_RIMC_ATTR_MPRIV,
           STM32_RIFSC_RIMC_ATTR(RIF_MASTER_INDEX_DCMIPP));

  /* DCMIPP peripheral slave attributes = SEC|PRIV.  The bare-metal also
   * calls HAL_RIF_RISC_SetSlaveSecureAttributes(DCMIPP, SEC|PRIV) for
   * RIF_RISC_PERIPH_INDEX_DCMIPP = RIF_PERIPH_REG2 | SEC29, i.e.
   * RISC_SECCFGR2 bit29 (RIFSC+0x18) and RISC_PRIVCFGR2 bit29 (RIFSC+0x38).
   */

  modifyreg32(STM32_RIFSC_BASE + 0x18, 0, (1u << 29));  /* SEC29 */
  modifyreg32(STM32_RIFSC_BASE + 0x38, 0, (1u << 29));  /* PRIV29 */
}

/****************************************************************************
 * Name: dcmipp_csi_write_phyreg
 *
 * Description:
 *   Write a register into the CSI D-PHY via the test control interface.
 *   Port of HAL DCMIPP_CSI_WritePHYReg.
 *
 ****************************************************************************/

static void dcmipp_csi_write_phyreg(uint32_t reg_msb, uint32_t reg_lsb,
                                    uint32_t val)
{
  /* For writing the 4-bit testcode MSBs: set testen + testclk high */

  CSI->PTCR1 |= CSI_PTCR1_TWM;
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  up_udelay(2);

  /* Set testclk low (falling edge latches testdin) */

  CSI->PTCR0 = 0;
  up_udelay(2);

  /* Set testen low */

  CSI->PTCR1 = 0;
  up_udelay(2);

  /* Place the testcode MSBs in testdin */

  CSI->PTCR1 = reg_msb & 0xff;
  up_udelay(2);

  /* Set testclk high */

  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  up_udelay(2);

  /* For writing the testcode LSBs: testclk low, testen high */

  CSI->PTCR0 = 0;
  up_udelay(2);
  CSI->PTCR1 |= CSI_PTCR1_TWM;
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  up_udelay(2);

  /* Place the testcode LSBs + write strobe in testdin */

  CSI->PTCR1 = CSI_PTCR1_TWM | (reg_lsb & 0xff);
  up_udelay(2);

  /* Testclk low latches the register address */

  CSI->PTCR0 = 0;
  up_udelay(2);
  CSI->PTCR1 = 0;
  up_udelay(2);

  /* Place the data in testdin */

  CSI->PTCR1 = val & 0xff;
  up_udelay(2);

  /* Testclk high programs the data */

  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  up_udelay(2);
  CSI->PTCR0 = 0;
  up_udelay(2);
}

/****************************************************************************
 * Name: dcmipp_csi_read_phyreg
 *
 * Description:
 *   Read back a register from the CSI D-PHY via the test control
 *   interface (read mode: TWM=0).  Used for diagnostics to verify the
 *   deskew / DLL configuration written at init actually landed in the
 *   D-PHY.
 *
 ****************************************************************************/

static uint32_t dcmipp_csi_read_phyreg(uint32_t reg_msb, uint32_t reg_lsb)
{
  uint32_t val;

  /* Mirror the write_phyreg TWM (testen) handshake 1->0->1->0 so the
   * test-interface transaction actually engages.  The previous version
   * held TWM=0 the whole time, which likely never started a read
   * transaction and therefore always returned 0x00. */

  /* Entry: testen high, testclk on, one clock edge */

  CSI->PTCR1 = CSI_PTCR1_TWM;
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  up_udelay(2);
  CSI->PTCR0 = 0;
  up_udelay(2);

  /* Address MSBs, testen low */

  CSI->PTCR1 = 0;
  CSI->PTCR1 = reg_msb & 0xff;
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  up_udelay(2);
  CSI->PTCR0 = 0;
  up_udelay(2);

  /* Address LSBs, testen high (matches the write strobe position) */

  CSI->PTCR1 = CSI_PTCR1_TWM | (reg_lsb & 0xff);
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  up_udelay(2);
  CSI->PTCR0 = 0;
  up_udelay(2);

  /* Read data: testen low, one clock edge, sample TDO */

  CSI->PTCR1 = 0;
  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  up_udelay(2);
  val = CSI->PTSR & CSI_PTSR_TDO;
  CSI->PTCR0 = 0;
  CSI->PTCR1 = 0;
  up_udelay(2);

  return val;
}

/****************************************************************************
 * Name: dcmipp_csi_set_config
 *
 * Description:
 *   Configure the CSI-2 receiver: 2 data lanes, physical lane mapping,
 *   PHY bitrate 1600 Mbps (BT_1600, matches bare-metal).  Port of
 *   HAL_DCMIPP_CSI_SetConfig.
 *
 ****************************************************************************/

static int dcmipp_csi_set_config(const struct dcmipp_csi_config_s *cfg)
{
  /* Ensure the CSI is disabled */

  CSI->CR &= ~CSI_CR_CSIEN;

  /* Configure the lane merger: LANENB=2 (two data lanes), DL0MAP=1
   * (logical DL0 -> physical lane 0), DL1MAP=2 (logical DL1 -> physical
   * lane 1) => LMCFGR = 0x210200.  NOTE: the CSI reset value is
   * 0x43210200 which ALSO sets reserved bits 30/25/24; the bare-metal
   * HAL clears those (writes 0x210200).  Keeping the reserved bits
   * breaks the D-PHY data lanes (DL0/DL1 no activity, SOT/ECC errors,
   * PIPE1 gets no frames).  Verified on real HW 2026-08-22.
   */

  CSI->LMCFGR = 0x210200u;

  /* Enable the CSI */

  CSI->CR |= CSI_CR_CSIEN;

  /* Enable the CSI error interrupts (not related to virtual channels) */

  CSI->IER0 |= DCMIPP_CSI_IER0_ENABLE;

  /* Enable the D-PHY interrupts (2 lanes) */

  CSI->IER1 |= DCMIPP_CSI_IER1_ENABLE;

  /* Start D-PHY configuration: stop the D-PHY, lanes disabled */

  CSI->PRCR &= ~CSI_PRCR_PEN;
  CSI->PCR = 0;

  /* Set the test clock enable on for ~15 ns */

  CSI->PTCR0 |= CSI_PTCR0_TCKEN;
  up_mdelay(1);
  CSI->PTCR0 = 0;

  /* Set hsfreqrange (and the configuration clock frequency range) */

  /* CCFR = 0x28 (matches the bare-metal appli PFCR=0x10D28).  Core basis
   * = follow bare-metal: replicate its exact PHY config for a fair test.
   */

  modifyreg32((uintptr_t)&CSI->PFCR, CSI_PFCR_HSFR,
              (DCMIPP_PHY_CCFR << CSI_PFCR_CCFR_SHIFT) |
              (DCMIPP_PHY_HSFR << CSI_PFCR_HSFR_SHIFT));

  /* D-PHY test registers.  Baseline values are the SNPS D-PHY defaults
   * used by HAL_DCMIPP_CSI_SetConfig (deskew 0x38, DLL prog 0x11, DLL osc
   * target 295).  g_dcmipp_rx_mode lets a single firmware A/B test RX
   * parameter variants to chase CSI SYNCERR (byte-level sync failure).
   */

  {
    uint32_t rx_osc  = DCMIPP_PHY_OSC;
    uint32_t rx_de   = 0x38u;

    if (g_dcmipp_rx_mode == 1)
      {
        rx_de = 0x00u;
      }
    else if (g_dcmipp_rx_mode == 2)
      {
        rx_de = 0x3fu;
      }
    else if (g_dcmipp_rx_mode == 3)
      {
        rx_osc = 285u;
      }
    else if (g_dcmipp_rx_mode == 4)
      {
        rx_osc = 305u;
      }

    dcmipp_csi_write_phyreg(0x00, 0x08, rx_de);         /* deskew polarity */
    dcmipp_csi_write_phyreg(0x00, 0xe4, 0x11);          /* DLL prog enable   */
    dcmipp_csi_write_phyreg(0x00, 0xe3, rx_osc >> 8);   /* DLL osc MSB       */
    dcmipp_csi_write_phyreg(0x00, 0xe3, rx_osc & 0xff); /* DLL osc LSB       */

  }

  /* Set base dir (RX) + frequency range */

  CSI->PFCR = (DCMIPP_PHY_CCFR << CSI_PFCR_CCFR_SHIFT) |
              (DCMIPP_PHY_HSFR << CSI_PFCR_HSFR_SHIFT) |
              CSI_PFCR_DLD;

  /* Enable the D-PHY RX lanes (2 data lanes + clock), power up */

  CSI->PCR = CSI_PCR_DL0EN | CSI_PCR_DL1EN | CSI_PCR_CLEN |
             CSI_PCR_PWRDOWN;

  /* Enable the PHY, out of reset */

  CSI->PRCR |= CSI_PRCR_PEN;

  /* Remove the force */

  CSI->PMCR = 0;

  return OK;
}

/****************************************************************************
 * Name: dcmipp_csi_set_vc_config
 *
 * Description:
 *   Set the common data type format for the selected virtual channel.
 *   Port of HAL_DCMIPP_CSI_SetVCConfig.
 *
 ****************************************************************************/

static void dcmipp_csi_set_vc_config(uint32_t vc, uint32_t data_format)
{
  switch (vc)
    {
      case DCMIPP_VIRTUAL_CHANNEL0:
        CSI->VC0CFGR1 = (data_format << CSI_VC0CFGR1_CDTFT_SHIFT) |
                        CSI_VC0CFGR1_ALLDT;

        /* VC0START is asserted in stm32n6_dcmipp_start_capture() once
         * the sensor is streaming (sensor-first, matching bare-metal).
         */

        break;

      case DCMIPP_VIRTUAL_CHANNEL1:
        CSI->VC1CFGR1 = (data_format << CSI_VC1CFGR1_CDTFT_SHIFT) |
                        CSI_VC1CFGR1_ALLDT;
        break;

      default:
        break;
    }
}

/****************************************************************************
 * Name: dcmipp_pipe1_flow_config
 *
 * Description:
 *   Configure PIPE1 data type filtering (RAW10 from VC0) and select the
 *   serial (CSI) input.  Port of HAL_DCMIPP_CSI_PIPE_SetConfig.
 *
 ****************************************************************************/

static void dcmipp_pipe1_flow_config(
              FAR const struct dcmipp_csi_pipe_config_s *cfg)
{
  uint32_t fscr;

  /* Data type mode + IDA (IDB not written for DTIDA-only mode) */

  fscr = cfg->datatype_mode |
         (cfg->datatype_ida << DCMIPP_P1FSCR_DTIDA_SHIFT) |
         (DCMIPP_VIRTUAL_CHANNEL0 << DCMIPP_P1FSCR_VC_SHIFT);

  modifyreg32((uintptr_t)&DCMIPP->P1FSCR,
              DCMIPP_P1FSCR_DTIDA | DCMIPP_P1FSCR_DTIDB |
              DCMIPP_P1FSCR_DTMODE | DCMIPP_P1FSCR_VC, fscr);

  /* Disable the parallel interface, select the CSI (serial) input */

  DCMIPP->PRCR &= ~DCMIPP_PRCR_ENABLE;
  DCMIPP->CMCR |= DCMIPP_CMCR_INSEL;
}

/****************************************************************************
 * Name: dcmipp_pipe1_config
 *
 * Description:
 *   Configure PIPE1 pixel packer (RGB565), pitch and frame rate.
 *   Port of HAL_DCMIPP_PIPE_SetConfig / Pipe_Config.
 *
 ****************************************************************************/

static void dcmipp_pipe1_config(const struct dcmipp_pipe_config_s *cfg)
{
  /* Frame rate: all frames */

  modifyreg32((uintptr_t)&DCMIPP->P1FCTCR, DCMIPP_P1FCTCR_FRATE,
              cfg->frame_rate);

  /* Pixel packer format (RGB565) */

  modifyreg32((uintptr_t)&DCMIPP->P1PPCR, DCMIPP_P1PPCR_FORMAT,
              cfg->pixel_format);

  /* Pixel pipe pitch (bytes per line).  LINEMULT is intentionally left
   * at its reset default (matches the bare-metal diag9f reference). */

  /* Pixel pipe pitch (bytes per line) */

  modifyreg32((uintptr_t)&DCMIPP->P1PPM0PR, DCMIPP_P1PPM0PR_PITCH,
              cfg->pixel_pitch << DCMIPP_P1PPM0PR_PITCH_SHIFT);
}

/****************************************************************************
 * Name: dcmipp_pipe1_downsize_config
 *
 * Description:
 *   Configure and enable the PIPE1 downsize (2592x1944 -> 800x480).
 *   Port of HAL_DCMIPP_PIPE_SetDownsizeConfig / EnableDownsize.
 *
 ****************************************************************************/

static void dcmipp_pipe1_downsize_config(
                            const struct dcmipp_downsize_config_s *cfg)
{
  /* Vertical and horizontal division factors */

  modifyreg32((uintptr_t)&DCMIPP->P1DSCR,
              DCMIPP_P1DSCR_HDIV | DCMIPP_P1DSCR_VDIV,
              (cfg->hdivfactor << DCMIPP_P1DSCR_HDIV_SHIFT) |
              (cfg->vdivfactor << DCMIPP_P1DSCR_VDIV_SHIFT));

  /* Vertical and horizontal ratios */

  putreg32((cfg->hratio << DCMIPP_P1DSRTIOR_HRATIO_SHIFT) |
           (cfg->vratio << DCMIPP_P1DSRTIOR_VRATIO_SHIFT),
           (uintptr_t)&DCMIPP->P1DSRTIOR);

  /* Downsize destination size */

  modifyreg32((uintptr_t)&DCMIPP->P1DSSZR,
              DCMIPP_P1DSSZR_HSIZE | DCMIPP_P1DSSZR_VSIZE,
              (cfg->hsize << DCMIPP_P1DSSZR_HSIZE_SHIFT) |
              (cfg->vsize << DCMIPP_P1DSSZR_VSIZE_SHIFT));

  /* Enable the downsize */

  DCMIPP->P1DSCR |= DCMIPP_P1DSCR_ENABLE;
}

/****************************************************************************
 * Name: dcmipp_pipe1_demosaic_config
 *
 * Description:
 *   Enable the PIPE1 hardware demosaic (Raw Bayer 10-bit to RGB).  The
 *   IMX335 outputs a RGGB Bayer pattern; without P1DMCR enabled the PIPE1
 *   pixel pipeline cannot produce RGB565 and does not consume the CSI
 *   data (the CSI clock-changer FIFO then stays full / CCFIFOFF).
 *   Strengths follow the ALIENTEK ISP IQ profile (peak=2, lineV=4,
 *   lineH=4, edge=6).  Port of HAL_DCMIPP_PIPE_SetISPRawBayer2RGBConfig
 *   + HAL_DCMIPP_PIPE_EnableISPRawBayer2RGB.
 *
 ****************************************************************************/

static void dcmipp_pipe1_demosaic_config(void)
{
  /* RGGB (TYPE = 0), default strengths */

  putreg32(DCMIPP_P1DMCR_ENABLE |
           (2u << DCMIPP_P1DMCR_PEAK_SHIFT) |
           (4u << DCMIPP_P1DMCR_LINEV_SHIFT) |
           (4u << DCMIPP_P1DMCR_LINEH_SHIFT) |
           (6u << DCMIPP_P1DMCR_EDGE_SHIFT),
           (uintptr_t)&DCMIPP->P1DMCR);
}

/****************************************************************************
 * Name: dcmipp_pipe1_colorconv_config
 *
 * Description:
 *   Enable the PIPE1 ColorConv (3x3 color matrix) with a static D65 white
 *   balance.  The IMX335 RAW10 Bayer stream is green-ish without white
 *   balance correction (the ALIENTEK demo applies it in the evision AWB
 *   algorithm, which is not available in NuttX).  Apply the same D65
 *   gains from the 352_IMX335 ISP IQ profile (AWBAlgo.ispGain) as a
 *   diagonal matrix:  R x2.45   G x1.00   B x1.55
 *   Register format is Q8 (1.0 = 256), matching ISP_SVC_ISP_SetColorConv's
 *   To_CConv_Reg() conversion (verified on bare metal 2026-08-22).
 *
 ****************************************************************************/

static void dcmipp_pipe1_colorconv_config(void)
{
  /* Enable the ColorConv block (Clamp disabled, RGB output) */

  putreg32(DCMIPP_P1CCCR_ENABLE, (uintptr_t)&DCMIPP->P1CCCR);

  /* Row 1 (R out): RR = 2.45 (627), RG = 0, RB = 0, RA = 0 */

  putreg32((627u << DCMIPP_P1CCRR1_RR_SHIFT), (uintptr_t)&DCMIPP->P1CCRR1);
  putreg32(0, (uintptr_t)&DCMIPP->P1CCRR2);

  /* Row 2 (G out): GR = 0, GG = 1.00 (256), GB = 0, GA = 0 */

  putreg32((256u << DCMIPP_P1CCGR1_GG_SHIFT), (uintptr_t)&DCMIPP->P1CCGR1);
  putreg32(0, (uintptr_t)&DCMIPP->P1CCGR2);

  /* Row 3 (B out): BR = 0, BG = 0, BB = 1.55 (397), BA = 0 */

  putreg32(0, (uintptr_t)&DCMIPP->P1CCBR1);
  putreg32((397u << DCMIPP_P1CCBR2_BB_SHIFT), (uintptr_t)&DCMIPP->P1CCBR2);

  _info("dcmipp: PIPE1 ColorConv static WB applied (D65 R2.45 G1.00 B1.55)\n");
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_dcmipp_init
 *
 * Description:
 *   Initialize the DCMIPP + CSI-2 for the IMX335 sensor.  Must be called
 *   after the sensor is powered and I2C-configured (see imx335 driver).
 *
 ****************************************************************************/

int stm32n6_dcmipp_init(void)
{
  /* D-Cache is deliberately LEFT ENABLED here.
   *
   * EXPERIMENT (2026-08-23) used to call up_disable_dcache() at this point,
   * to test whether cache/bus activity was disturbing the CSI D-PHY lane
   * sync.  The cost of that experiment was system-wide and permanent:
   * stm32n6_dcmipp_init() runs on every stm32n6_video_start(), i.e. once per
   * captured frame, and nothing ever re-enabled the cache.  With it off,
   * up_mdelay(203) took 1.6 s, up_mdelay(50) took 480 ms and the eye crop
   * took 340 ms -- 3180 ms of capture loop for a 120 ms inference.
   *
   * The framebuffer is still coherent: the application invalidates the band
   * it is about to read after the DCMIPP DMA writes it, and cleans the band
   * it draws before the LTDC DMA scans it.
   */

  _info("dcmipp: D-Cache left enabled (NVIC_CFGCON=0x%08lx)\n",
        (unsigned long)getreg32(NVIC_CFGCON));

  /* phy_bitrate is informational only: dcmipp_csi_set_config() hardcodes
   * the BT_1600 PHY register values (HSFR=0x0D / OSC=295 / CCFR=0x28) to
   * match the bare-metal reference (PFCR=0x10D28).  Keep this field at
   * BT_1600 so it cannot be mistaken for a different runtime bitrate. */

  const struct dcmipp_csi_config_s csi_cfg =
  {
    .num_lanes    = DCMIPP_CSI_TWO_DATA_LANES,
    .lane_mapping = DCMIPP_CSI_PHYSICAL_DATA_LANES,
    .phy_bitrate  = DCMIPP_CSI_PHY_BT_1600
  };

  /* Data type filter: RAW10 (0x2B) only.  IMX335 0x319D[0]=MDBIT=0
   * confirms 10-bit output, so DTMODE_DTIDA with DT_RAW10 matches the
   * sensor (verified against the datasheet 0x319D table). */
  const struct dcmipp_csi_pipe_config_s pipe_flow_cfg =
  {
    .datatype_mode = DCMIPP_DTMODE_DTIDA,
    .datatype_ida  = DCMIPP_DT_RAW10,
    .datatype_idb  = DCMIPP_DT_RAW10
  };

  const struct dcmipp_pipe_config_s pipe_cfg =
  {
    .frame_rate   = DCMIPP_FRAME_RATE_ALL,
    .pixel_pitch  = 800 * 2,                 /* RGB565, 800 px/line */
    .pixel_format = DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1
  };

  const struct dcmipp_downsize_config_s down_cfg =
  {
    .vsize       = 480,
    .hsize       = 800,
    .vratio      = 33161,
    .hratio      = 25656,
    .vdivfactor  = 253,
    .hdivfactor  = 316
  };

  int ret;

  /* Clocks + RIF master attributes */

  dcmipp_clock_config();
  dcmipp_rif_config();

  /* Attach the DCMIPP / CSI interrupt handlers */

  ret = irq_attach(STM32_IRQ_DCMIPP, stm32n6_dcmipp_isr, NULL);
  if (ret < 0)
    {
      _err("dcmipp: IRQ attach DCMIPP failed: %d\n", ret);
      return ret;
    }

  ret = irq_attach(STM32_IRQ_CSI, stm32n6_dcmipp_csi_isr, NULL);
  if (ret < 0)
    {
      _err("dcmipp: IRQ attach CSI failed: %d\n", ret);
      return ret;
    }

  up_enable_irq(STM32_IRQ_DCMIPP);
  up_enable_irq(STM32_IRQ_CSI);

  /* CSI-2 receiver */

  ret = dcmipp_csi_set_config(&csi_cfg);
  if (ret < 0)
    {
      return ret;
    }

  /* VC0: 10-bit data type format */

  dcmipp_csi_set_vc_config(DCMIPP_VIRTUAL_CHANNEL0, DCMIPP_CSI_DT_BPP10);

  /* PIPE1: filter RAW10 from VC0, select serial input */

  dcmipp_pipe1_flow_config(&pipe_flow_cfg);

  /* PIPE1: pixel packer RGB565 + pitch */

  dcmipp_pipe1_config(&pipe_cfg);

  /* PIPE1: downsize 2592x1944 -> 800x480 */

  dcmipp_pipe1_downsize_config(&down_cfg);

  /* PIPE1: demosaic + ColorConv are enabled LATE (just before PIPE1
   * activation in start_capture), matching the bare-metal ISP_Start
   * timing.  Enabling them at init left them armed for hundreds of ms
   * before PIPE1 ever started, which appeared to disturb the PIPE1
   * data flow once capture began (FIFO overflow / SYNCERR ~500ms in). */


  g_dcmipp.frame_count = 0;
  g_dcmipp.error_code = 0;
  g_dcmipp.overrun_count = 0;
  g_dcmipp.csi_err_count = 0;

  _info("dcmipp: initialized (CSI 2-lane PHY1600 VC0 RAW10 -> "
        "PIPE1 RGB565 800x480)\n");

  return OK;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_start_capture
 *
 * Description:
 *   Start PIPE1 continuous capture into dstaddr (16-byte aligned).
 *   SENSOR-FIRST order (matches the bare-metal diag9f reference): the
 *   sensor must already be streaming (stm32n6_imx335_start_stream() ran
 *   first).  VC0 is started, then PIPE1 is armed; VC0STATEF and the first
 *   SOF are awaited so the caller knows the link is live.
 *
 ****************************************************************************/

int stm32n6_dcmipp_start_capture(uint32_t dstaddr)
{
  clock_t start;

  if ((dstaddr & 0x0fu) != 0u)
    {
      _err("dcmipp: destination not 16-byte aligned: 0x%08lx\n",
           (unsigned long)dstaddr);
      return -EINVAL;
    }

  /* Reset the sticky diagnostics for this run */

  g_dcmipp.error_code    = 0;
  g_dcmipp.csi_err_count = 0;
  g_dcmipp.overrun_count = 0;

  /* --- Step 1: configure PIPE1 (ISP chain + destination), do NOT arm
   * it yet.  Matches the bare-metal ISP_Start order: demosaic ->
   * decimation(f1) -> black level -> stats window -> gamma -> colorconv.
   * --- */

  DCMIPP->P1FCTCR |= 0;                  /* CPTMODE=0: continuous */
  putreg32(dstaddr, (uintptr_t)&DCMIPP->P1PPM0AR1);

  DCMIPP->CMIER |= DCMIPP_PIPE1_IT_ENABLE;
  DCMIPP->CMFCR = DCMIPP_CMSR2_P1OVRF;

  /* ISP chain, matching the bare-metal ISP_Start values exactly
   * (verified bit-for-bit against diag9f runtime dump):
   * P1DMCR=0x64420001 P1DECR=0x1 P1BLCCR=0x0C0C0C01
   * P1STSZR=0x87980A20 P1GMCR=0x1 P1CCCR=0x1. */
  dcmipp_pipe1_demosaic_config();
  putreg32(DCMIPP_P1DECR_ENABLE, (uintptr_t)&DCMIPP->P1DECR);

  putreg32(DCMIPP_P1BLCCR_ENABLE |
           (12u << DCMIPP_P1BLCCR_BLCR_Pos) |
           (12u << DCMIPP_P1BLCCR_BLCG_Pos) |
           (12u << DCMIPP_P1BLCCR_BLCB_Pos),
           (uintptr_t)&DCMIPP->P1BLCCR);

  putreg32(0u, (uintptr_t)&DCMIPP->P1STSTR);   /* HStart=0, VStart=0 */

  putreg32(DCMIPP_P1STSZR_CROPEN |
           (2592u << DCMIPP_P1STSZR_HSIZE_Pos) |
           (1944u << DCMIPP_P1STSZR_VSIZE_Pos),
           (uintptr_t)&DCMIPP->P1STSZR);

  DCMIPP->P1GMCR |= DCMIPP_P1GMCR_ENABLE;

  dcmipp_pipe1_colorconv_config();

  /* --- Step 2 (EXPERIMENT I: CAPTURE-FIRST): clear stale CSI flags and
   * start VC0.  The sensor is NOT streaming yet (start_stream runs after
   * this call) - matching the earlier P0 experiment that first moved data
   * (frames=1).  With the INCK 37MHz configuration now correct, this order
   * lets the D-PHY lock onto the signal as it arrives, instead of sitting
   * enabled for a long time with no signal and then failing to sync
   * (SYNCERR/ECTRLDL seen in sensor-first runs).  VC0STATEF is awaited by
   * stm32n6_dcmipp_wait_first_frame() after the sensor starts. --- */

  CSI->FCR0 = 0xffffffffu;
  CSI->FCR1 = 0xffffffffu;

  CSI->CR |= CSI_CR_VC0START;

  /* --- Step 3: arm PIPE1 now (PIPEN + CPTREQ) so it consumes the moment
   * data arrives. --- */

  DCMIPP->P1FSCR |= DCMIPP_P1FSCR_PIPEN;
  DCMIPP->P1FCTCR |= DCMIPP_P1FCTCR_CPTREQ;

  _info("dcmipp: capture-first armed: SR0=0x%08lx SR1=0x%08lx\n",
        (unsigned long)CSI->SR0, (unsigned long)CSI->SR1);

  /* --- Step 4: enable ONLY the SOF/EOF interrupts (error interrupts
   * disabled to avoid the ISR storm while the link is unstable). --- */

  CSI->IER0 |= CSI_IER0_SOF0IE | CSI_IER0_EOF0IE;
  CSI->IER1 = 0u;
  CSI->FCR0 = 0xffffffffu;
  CSI->FCR1 = 0xffffffffu;

  _info("dcmipp: PIPE1 capture armed -> 0x%08lx\n",
        (unsigned long)dstaddr);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_wait_first_frame
 *
 * Description:
 *   Wait for VC0STATEF + first SOF.  Used in CAPTURE-FIRST order: the
 *   sensor starts streaming after stm32n6_dcmipp_start_capture(), so
 *   VC0STATEF only asserts once real data arrives from the sensor.
 *
 ****************************************************************************/

int stm32n6_dcmipp_wait_first_frame(void)
{
  clock_t start;

  /* Wait for VC0STATEF (sensor data arriving) */

  start = clock_systime_ticks();
  while ((CSI->SR0 & CSI_SR0_VC0STATEF) == 0u)
    {
      if (clock_systime_ticks() - start > MSEC2TICK(1000))
        {
          _err("dcmipp: VC0STATEF timeout SR0=0x%08lx SR1=0x%08lx\n",
               (unsigned long)CSI->SR0, (unsigned long)CSI->SR1);
          return -ETIMEDOUT;
        }
    }

  _info("dcmipp: VC0 active SR0=0x%08lx SR1=0x%08lx\n",
        (unsigned long)CSI->SR0, (unsigned long)CSI->SR1);

  /* Wait for the first PIPE1 frame: poll on the ISR frame counter
   * (incremented on P1FRAMEF).  Polling the CSI SOF0F sticky flag is
   * unreliable here: the CSI ISR clears FCR0=SR0 on every interrupt, so
   * the poll can never observe SOF0F even though frames are flowing. */

  start = clock_systime_ticks();
  while (g_dcmipp.frame_count == 0u)
    {
      if (clock_systime_ticks() - start > MSEC2TICK(1000))
        {
          _err("dcmipp: first-frame timeout SR0=0x%08lx SR1=0x%08lx "
               "ERR1=0x%08lx ERR2=0x%08lx SPDFR=0x%08lx\n",
               (unsigned long)CSI->SR0, (unsigned long)CSI->SR1,
               (unsigned long)CSI->ERR1, (unsigned long)CSI->ERR2,
               (unsigned long)CSI->SPDFR);
          _err("dcmipp:   IER0=0x%08lx IER1=0x%08lx P1FSCR=0x%08lx "
               "CMSR1=0x%08lx CMSR2=0x%08lx\n",
               (unsigned long)CSI->IER0, (unsigned long)CSI->IER1,
               (unsigned long)DCMIPP->P1FSCR,
               (unsigned long)DCMIPP->CMSR1, (unsigned long)DCMIPP->CMSR2);
          return -ETIMEDOUT;
        }
    }

  _info("dcmipp: first frame received, frame_count=%lu\n",
        (unsigned long)g_dcmipp.frame_count);

  return OK;
}
/****************************************************************************
 * Name: stm32n6_dcmipp_stop
 *
 ****************************************************************************/

int stm32n6_dcmipp_stop(void)
{
  clock_t start;

  /* Stop the capture request */

  DCMIPP->P1FCTCR &= ~DCMIPP_P1FCTCR_CPTREQ;

  /* Poll until no capture is active (timeout-guarded) */

  start = clock_systime_ticks();
  while ((DCMIPP->CMSR1 & DCMIPP_CMSR1_P1CPTACT) != 0u)
    {
      if (clock_systime_ticks() - start >
          MSEC2TICK(DCMIPP_STOP_TIMEOUT))
        {
          _err("dcmipp: stop timeout\n");
          return -ETIMEDOUT;
        }
    }

  _info("dcmipp: PIPE1 capture stopped\n");

  return OK;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_get_frame_count
 *
 ****************************************************************************/

uint32_t stm32n6_dcmipp_get_frame_count(void)
{
  return g_dcmipp.frame_count;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_get_csi_sr0
 *
 * Description:
 *   Return the current CSI-2 SR0 status register (diagnostics: SOF0F /
 *   EOF0F / VC0STATEF tell whether the sensor is streaming).
 *
 ****************************************************************************/

uint32_t stm32n6_dcmipp_get_csi_sr0(void)
{
  return CSI->SR0;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_get_csi_sr1
 *
 * Description:
 *   Return the current CSI-2 SR1 status register (diagnostics: SYNCDL0F /
 *   SYNCDL1F / ACTDL0F / ACTDL1F tell whether each data lane has locked
 *   and is receiving high-speed data).
 *
 ****************************************************************************/

uint32_t stm32n6_dcmipp_get_csi_sr1(void)
{
  return CSI->SR1;
}

uint32_t stm32n6_dcmipp_get_err1(void)
{
  return CSI->ERR1;
}

uint32_t stm32n6_dcmipp_get_err2(void)
{
  return CSI->ERR2;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_get_p1fctcr / get_cmsr1 / get_cmsr2
 *
 * Description:
 *   Read-only accessors for the PIPE1 flow-control and common status
 *   registers, used by the cam application's timeline diagnostic to
 *   pinpoint WHEN CPTREQ/P1CPTACT collapse.
 *
 ****************************************************************************/

uint32_t stm32n6_dcmipp_get_p1fctcr(void)
{
  return DCMIPP->P1FCTCR;
}

uint32_t stm32n6_dcmipp_get_cmsr1(void)
{
  return DCMIPP->CMSR1;
}

uint32_t stm32n6_dcmipp_get_cmsr2(void)
{
  return DCMIPP->CMSR2;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_clear_csi_flags
 *
 * Description:
 *   Clear CSI-2 status flags (FCR0/FCR1 write-1-to-clear).  Used by the
 *   long-duration frame-event monitor to reset the sticky SOF/EOF/VC0
 *   flags so each received frame can be counted.
 *
 ****************************************************************************/

void stm32n6_dcmipp_clear_csi_flags(uint32_t sr0_mask, uint32_t sr1_mask)
{
  CSI->FCR0 = sr0_mask;
  CSI->FCR1 = sr1_mask;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_get_overrun_count
 *
 ****************************************************************************/

uint32_t stm32n6_dcmipp_get_overrun_count(void)
{
  return g_dcmipp.overrun_count;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_get_error_code
 *
 * Description:
 *   Return the sticky CSI-2 error mask accumulated since the last start.
 *
 ****************************************************************************/

uint32_t stm32n6_dcmipp_get_error_code(void)
{
  return g_dcmipp.error_code;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_recover_vc0
 *
 * Description:
 *   Diagnostic recovery: clear all CSI-2 flags, re-arm the error
 *   interrupts and re-assert VC0START.  If the sensor is still streaming
 *   VC0STATEF re-asserts and PIPE1 resumes; if it times out the sensor
 *   has stopped (or the D-PHY is wedged beyond flag recovery).
 *
 ****************************************************************************/

int stm32n6_dcmipp_recover_vc0(void)
{
  clock_t start;

  CSI->FCR0 = 0xffffffffu;
  CSI->FCR1 = 0xffffffffu;
  CSI->IER0 |= DCMIPP_CSI_IER0_ENABLE | CSI_IER0_SOF0IE | CSI_IER0_EOF0IE;
  CSI->IER1 |= DCMIPP_CSI_IER1_ENABLE;

  CSI->CR |= CSI_CR_VC0START;

  start = clock_systime_ticks();
  while ((CSI->SR0 & CSI_SR0_VC0STATEF) == 0u)
    {
      if (clock_systime_ticks() - start > MSEC2TICK(500))
        {
          _err("dcmipp: VC0STATEF timeout on recover "
               "(sensor likely stopped)\n");
          return -ETIMEDOUT;
        }
    }

  _info("dcmipp: VC0 re-activated, SR0=0x%08lx SR1=0x%08lx\n",
        (unsigned long)CSI->SR0, (unsigned long)CSI->SR1);
  return OK;
}
/****************************************************************************
 * Name: stm32n6_dcmipp_set_frame_callback
 *
 ****************************************************************************/

void stm32n6_dcmipp_set_frame_callback(dcmipp_frame_cb_t cb)
{
  g_dcmipp.frame_cb = cb;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_isr
 *
 * Description:
 *   DCMIPP global interrupt handler (PIPE events + errors).
 *
 ****************************************************************************/

int stm32n6_dcmipp_isr(int irq, void *context, void *arg)
{
  uint32_t flags = DCMIPP->CMSR2;
  uint32_t ies   = DCMIPP->CMIER;

  /* PIPE1 line event (not enabled in this driver, clear anyway) */

  if ((flags & DCMIPP_CMSR2_P1LINEF) != 0u &&
      (ies & DCMIPP_CMIER_P1LINEIE) != 0u)
    {
      DCMIPP->CMFCR = DCMIPP_CMSR2_P1LINEF;
    }

  /* PIPE1 vsync event */

  if ((flags & DCMIPP_CMSR2_P1VSYNCF) != 0u &&
      (ies & DCMIPP_CMIER_P1VSYNCIE) != 0u)
    {
      DCMIPP->CMFCR = DCMIPP_CMSR2_P1VSYNCF;
    }

  /* PIPE1 frame event: count + callback */

  if ((flags & DCMIPP_CMSR2_P1FRAMEF) != 0u &&
      (ies & DCMIPP_CMIER_P1FRAMEIE) != 0u)
    {
      DCMIPP->CMFCR = DCMIPP_CMSR2_P1FRAMEF;

      g_dcmipp.frame_count++;

      /* NOTE: the P1OVR interrupt is NOT re-enabled here.  The bare-metal
       * HAL_DCMIPP_IRQHandler disables the overrun interrupt on the first
       * overrun (CONTINUOUS mode) and tolerates the dropped pixels.
       */

      if (g_dcmipp.frame_cb != NULL)
        {
          g_dcmipp.frame_cb(DCMIPP_PIPE1);
        }
    }

  /* PIPE1 overrun: match the bare-metal HAL_DCMIPP_IRQHandler - in
   * CONTINUOUS mode the overrun interrupt is DISABLED on the first
   * event and the pipe keeps capturing (dropped output pixels are
   * tolerated).  Keeping it armed caused an interrupt storm.
   */

  if ((flags & DCMIPP_CMSR2_P1OVRF) != 0u &&
      (ies & DCMIPP_CMIER_P1OVRIE) != 0u)
    {
      DCMIPP->CMIER &= ~DCMIPP_CMIER_P1OVRIE;
      DCMIPP->CMFCR = DCMIPP_CMSR2_P1OVRF;
      g_dcmipp.overrun_count++;

      if (g_dcmipp.overrun_count == 1u)
        {
          _err("dcmipp: PIPE1 overrun (IRQ disabled, tolerated)\n");
        }
    }

  /* IPPLUG AXI transfer error */

  if ((flags & DCMIPP_CMSR2_ATXERRF) != 0u &&
      (ies & DCMIPP_CMIER_ATXERRIE) != 0u)
    {
      DCMIPP->CMIER &= ~DCMIPP_CMIER_ATXERRIE;
      DCMIPP->CMFCR = DCMIPP_CMSR2_ATXERRF;
      g_dcmipp.error_code |= DCMIPP_CMIER_ATXERRIE;
      _err("dcmipp: AXI transfer error\n");
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_csi_isr
 *
 * Description:
 *   CSI-2 host interrupt handler: log and clear error flags.
 *
 ****************************************************************************/

int stm32n6_dcmipp_csi_isr(int irq, void *context, void *arg)
{
  uint32_t sr0 = CSI->SR0;
  uint32_t sr1 = CSI->SR1;
  uint32_t err = 0u;

  /* Match the bare-metal HAL_DCMIPP_CSI_IRQHandler exactly: for each
   * CSI-2 error flag, if set, disable its interrupt and clear the flag
   * IMMEDIATELY (write-1-to-clear).  The clock-changer FIFO may stay
   * "full" if CCFIFOFF is not cleared promptly, which blocks the data
   * path and causes the sensor to stop transmitting.
   */

  /* --- SR0 error flags (IER0) --- */

  if ((sr0 & CSI_SR0_CCFIFOFF) != 0u)
    {
      if ((CSI->IER0 & CSI_IER0_CCFIFOFIE) != 0u)
        {
          CSI->IER0 &= ~CSI_IER0_CCFIFOFIE;
          err |= CSI_SR0_CCFIFOFF;
        }

      CSI->FCR0 = CSI_SR0_CCFIFOFF;
    }

  if ((sr0 & CSI_SR0_SYNCERRF) != 0u)
    {
      if ((CSI->IER0 & CSI_IER0_SYNCERRIE) != 0u)
        {
          CSI->IER0 &= ~CSI_IER0_SYNCERRIE;
          err |= CSI_SR0_SYNCERRF;
        }

      CSI->FCR0 = CSI_SR0_SYNCERRF;
    }

  if ((sr0 & CSI_SR0_SPKTERRF) != 0u)
    {
      if ((CSI->IER0 & CSI_IER0_SPKTERRIE) != 0u)
        {
          CSI->IER0 &= ~CSI_IER0_SPKTERRIE;
          err |= CSI_SR0_SPKTERRF;
        }

      CSI->FCR0 = CSI_SR0_SPKTERRF;
    }

  if ((sr0 & CSI_SR0_IDERRF) != 0u)
    {
      if ((CSI->IER0 & CSI_IER0_IDERRIE) != 0u)
        {
          CSI->IER0 &= ~CSI_IER0_IDERRIE;
          err |= CSI_SR0_IDERRF;
        }

      CSI->FCR0 = CSI_SR0_IDERRF;
    }

  if ((sr0 & CSI_SR0_ECCERRF) != 0u)
    {
      if ((CSI->IER0 & CSI_IER0_ECCERRIE) != 0u)
        {
          CSI->IER0 &= ~CSI_IER0_ECCERRIE;
          err |= CSI_SR0_ECCERRF;
        }

      CSI->FCR0 = CSI_SR0_ECCERRF;
    }

  if ((sr0 & CSI_SR0_CECCERRF) != 0u)
    {
      if ((CSI->IER0 & CSI_IER0_CECCERRIE) != 0u)
        {
          CSI->IER0 &= ~CSI_IER0_CECCERRIE;
          err |= CSI_SR0_CECCERRF;
        }

      CSI->FCR0 = CSI_SR0_CECCERRF;
    }

  if ((sr0 & CSI_SR0_CRCERRF) != 0u)
    {
      if ((CSI->IER0 & CSI_IER0_CRCERRIE) != 0u)
        {
          CSI->IER0 &= ~CSI_IER0_CRCERRIE;
          err |= CSI_SR0_CRCERRF;
        }

      CSI->FCR0 = CSI_SR0_CRCERRF;
    }

  /* --- SR1 lane / D-PHY error flags (IER1) --- */

  if ((sr1 & CSI_SR1_ESOTDL0F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_ESOTDL0IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_ESOTDL0IE;
          err |= CSI_SR1_ESOTDL0F;
        }

      CSI->FCR1 = CSI_SR1_ESOTDL0F;
    }

  if ((sr1 & CSI_SR1_ESOTSYNCDL0F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_ESOTSYNCDL0IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_ESOTSYNCDL0IE;
          err |= CSI_SR1_ESOTSYNCDL0F;
        }

      CSI->FCR1 = CSI_SR1_ESOTSYNCDL0F;
    }

  if ((sr1 & CSI_SR1_EESCDL0F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_EESCDL0IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_EESCDL0IE;
          err |= CSI_SR1_EESCDL0F;
        }

      CSI->FCR1 = CSI_SR1_EESCDL0F;
    }

  if ((sr1 & CSI_SR1_ESYNCESCDL0F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_ESYNCESCDL0IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_ESYNCESCDL0IE;
          err |= CSI_SR1_ESYNCESCDL0F;
        }

      CSI->FCR1 = CSI_SR1_ESYNCESCDL0F;
    }

  if ((sr1 & CSI_SR1_ECTRLDL0F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_ECTRLDL0IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_ECTRLDL0IE;
          err |= CSI_SR1_ECTRLDL0F;
        }

      CSI->FCR1 = CSI_SR1_ECTRLDL0F;
    }

  if ((sr1 & CSI_SR1_ESOTDL1F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_ESOTDL1IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_ESOTDL1IE;
          err |= CSI_SR1_ESOTDL1F;
        }

      CSI->FCR1 = CSI_SR1_ESOTDL1F;
    }

  if ((sr1 & CSI_SR1_ESOTSYNCDL1F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_ESOTSYNCDL1IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_ESOTSYNCDL1IE;
          err |= CSI_SR1_ESOTSYNCDL1F;
        }

      CSI->FCR1 = CSI_SR1_ESOTSYNCDL1F;
    }

  if ((sr1 & CSI_SR1_EESCDL1F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_EESCDL1IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_EESCDL1IE;
          err |= CSI_SR1_EESCDL1F;
        }

      CSI->FCR1 = CSI_SR1_EESCDL1F;
    }

  if ((sr1 & CSI_SR1_ESYNCESCDL1F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_ESYNCESCDL1IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_ESYNCESCDL1IE;
          err |= CSI_SR1_ESYNCESCDL1F;
        }

      CSI->FCR1 = CSI_SR1_ESYNCESCDL1F;
    }

  if ((sr1 & CSI_SR1_ECTRLDL1F) != 0u)
    {
      if ((CSI->IER1 & CSI_IER1_ECTRLDL1IE) != 0u)
        {
          CSI->IER1 &= ~CSI_IER1_ECTRLDL1IE;
          err |= CSI_SR1_ECTRLDL1F;
        }

      CSI->FCR1 = CSI_SR1_ECTRLDL1F;
    }

  /* Record the sticky error mask and log the first event only */

  /* Record the sticky error mask and latch the first event so it can be
   * reported from task context.  Printing must NOT happen inside this
   * handler: the interrupt stack is small and sits directly in front of
   * g_mmheap, so a deep printf call chain can overrun it and silently
   * corrupt the heap.  See the status dump in the ioctl() path.
   */

  if (err != 0u)
    {
      g_dcmipp.error_code |= err;

      if (g_dcmipp.csi_err_count == 0)
        {
          g_dcmipp.csi_err_sr0 = sr0;
          g_dcmipp.csi_err_sr1 = sr1;
        }

      g_dcmipp.csi_err_count++;
    }

  /* Re-read SR0/SR1 and clear any remaining event flags (SOF/EOF/SPKT
   * etc.) that were set but not handled above.
   */

  CSI->FCR0 = CSI->SR0;
  CSI->FCR1 = CSI->SR1;

  return OK;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_dump_status
 *
 * Description:
 *   Dump the CSI-2 and DCMIPP status registers for diagnostics.
 *   Used by the 'cam diag' NSH command.
 *
 ****************************************************************************/

void stm32n6_dcmipp_set_rx_mode(int mode)
{
  g_dcmipp_rx_mode = mode;
}

int stm32n6_dcmipp_get_rx_mode(void)
{
  return g_dcmipp_rx_mode;
}

/****************************************************************************
 * Name: stm32n6_dcmipp_set_phy_bitrate
 *
 * Description:
 *   Runtime D-PHY bitrate switch (diagnostic): stop the PHY, reprogram
 *   HSFR + DLL oscillator target, restart.  Lets a single firmware A/B
 *   test whether the CSI data-lane sync (SYNCDL) depends on the PHY
 *   bitrate matching the sensor's actual MIPI output (the Zephyr driver
 *   picks the bitrate from the sensor link-frequency; NuttX hardcodes
 *   BT_1600 which matches the bare-metal reference).
 *
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_dcmipp_phy_reinit
 *
 * Description:
 *   Re-run the D-PHY configuration while the sensor is already streaming,
 *   so the DLL / deskew calibrate against the REAL HS clock/data instead
 *   of the no-signal idle state they saw at init.  If the PHY never locks
 *   (CRC errors, no SOF), this gives it a clean restart on live signal.
 *
 ****************************************************************************/

int stm32n6_dcmipp_phy_reinit(void)
{
  /* Stop the D-PHY (lanes off) */

  CSI->PRCR &= ~CSI_PRCR_PEN;
  CSI->PCR = 0;

  /* Re-program the frequency range + DLL oscillator target + deskew */

  CSI->PFCR = (DCMIPP_PHY_CCFR << CSI_PFCR_CCFR_SHIFT) |
              (DCMIPP_PHY_HSFR << CSI_PFCR_HSFR_SHIFT) |
              CSI_PFCR_DLD;

  dcmipp_csi_write_phyreg(0x00, 0x08, 0x38);          /* deskew polarity  */
  dcmipp_csi_write_phyreg(0x00, 0xe4, 0x11);          /* DLL prog enable   */
  dcmipp_csi_write_phyreg(0x00, 0xe3, DCMIPP_PHY_OSC >> 8);   /* osc MSB */
  dcmipp_csi_write_phyreg(0x00, 0xe3, DCMIPP_PHY_OSC & 0xff); /* osc LSB */

  /* Re-enable the D-PHY RX lanes, out of reset */

  CSI->PCR = CSI_PCR_DL0EN | CSI_PCR_DL1EN | CSI_PCR_CLEN |
             CSI_PCR_PWRDOWN;
  CSI->PRCR |= CSI_PRCR_PEN;

  up_mdelay(20);

  _info("dcmipp: PHY re-initialized on live sensor signal "
        "(HSFR=0x%02lx osc=%lu)\n",
        (unsigned long)DCMIPP_PHY_HSFR, (unsigned long)DCMIPP_PHY_OSC);

  return OK;
}

int stm32n6_dcmipp_set_phy_bitrate(int mbps)
{
  uint32_t hsfr;
  uint32_t osc;

  switch (mbps)
    {
      case 1000:
        hsfr = 0x0au;
        osc  = 460u;
        break;
      case 1200:
        hsfr = 0x0bu;
        osc  = 460u;
        break;
      case 1600:
        hsfr = 0x0du;
        osc  = 295u;
        break;
      default:
        return -EINVAL;
    }

  /* Stop the D-PHY (lanes off) */

  CSI->PRCR &= ~CSI_PRCR_PEN;
  CSI->PCR = 0;

  /* Re-program the frequency range + DLL oscillator target */

  CSI->PFCR = (DCMIPP_PHY_CCFR << CSI_PFCR_CCFR_SHIFT) |
              (hsfr << CSI_PFCR_HSFR_SHIFT) |
              CSI_PFCR_DLD;

  dcmipp_csi_write_phyreg(0x00, 0xe4, 0x11);       /* DLL prog enable */
  dcmipp_csi_write_phyreg(0x00, 0xe3, osc >> 8);   /* DLL osc MSB */
  dcmipp_csi_write_phyreg(0x00, 0xe3, osc & 0xff); /* DLL osc LSB */

  /* Re-enable the D-PHY RX lanes, out of reset */

  CSI->PCR = CSI_PCR_DL0EN | CSI_PCR_DL1EN | CSI_PCR_CLEN |
             CSI_PCR_PWRDOWN;
  CSI->PRCR |= CSI_PRCR_PEN;

  up_mdelay(10);

  _info("dcmipp: PHY bitrate set to %d Mbps (HSFR=0x%02lx osc=%lu)\n",
        mbps, (unsigned long)hsfr, (unsigned long)osc);

  return OK;
}

void stm32n6_dcmipp_dump_status(void)
{

  _info("dcmipp: --- CSI-2 status ---\n");
  _info("dcmipp:   IC17CFGR=0x%08lx IC18CFGR=0x%08lx\n",
        (unsigned long)getreg32(STM32_RCC_IC17CFGR),
        (unsigned long)getreg32(STM32_RCC_IC18CFGR));
  _info("dcmipp:   CR=0x%08lx PCR=0x%08lx LMCFGR=0x%08lx\n",
        (unsigned long)CSI->CR, (unsigned long)CSI->PCR,
        (unsigned long)CSI->LMCFGR);
  _info("dcmipp:   SR0=0x%08lx SR1=0x%08lx\n",
        (unsigned long)CSI->SR0, (unsigned long)CSI->SR1);
  _info("dcmipp:   IER0=0x%08lx IER1=0x%08lx\n",
        (unsigned long)CSI->IER0, (unsigned long)CSI->IER1);
  _info("dcmipp:   VC0CFGR1=0x%08lx PFCR=0x%08lx\n",
        (unsigned long)CSI->VC0CFGR1, (unsigned long)CSI->PFCR);

  _info("dcmipp:   ERR1=0x%08lx ERR2=0x%08lx SPDFR=0x%08lx\n",
        (unsigned long)CSI->ERR1, (unsigned long)CSI->ERR2,
        (unsigned long)CSI->SPDFR);
  _info("dcmipp:   SPDFR: field=0x%04lx dtype=0x%02lx vc=%lu\n",
        (unsigned long)(CSI->SPDFR & CSI_SPDFR_DATAFIELD),
        (unsigned long)((CSI->SPDFR & CSI_SPDFR_DATATYPE) \
                        >> CSI_SPDFR_DATATYPE_Pos),
        (unsigned long)((CSI->SPDFR & CSI_SPDFR_VCHANNEL) \
                        >> CSI_SPDFR_VCHANNEL_Pos));
  _info("dcmipp:   PRCR=0x%08lx PMCR=0x%08lx PTCR0=0x%08lx PTCR1=0x%08lx\n",
        (unsigned long)CSI->PRCR, (unsigned long)CSI->PMCR,
        (unsigned long)CSI->PTCR0, (unsigned long)CSI->PTCR1);
  _info("dcmipp:   PLL1CFGR1=0x%08lx PLL1CFGR3=0x%08lx\n",
        (unsigned long)getreg32(STM32_RCC_PLL1CFGR1),
        (unsigned long)getreg32(STM32_RCC_PLL1CFGR3));
  _info("dcmipp:   CSI err IRQ count: %d\n", g_dcmipp.csi_err_count);
  if (g_dcmipp.csi_err_count > 0)
    {
      _info("dcmipp:   CSI err[1st] SR0=0x%08lx SR1=0x%08lx\n",
            (unsigned long)g_dcmipp.csi_err_sr0,
            (unsigned long)g_dcmipp.csi_err_sr1);
    }

  _info("dcmipp:   sticky err code: 0x%08lx\n",
        (unsigned long)g_dcmipp.error_code);
  _info("dcmipp: --- DCMIPP status ---\n");
  _info("dcmipp:   CMCR=0x%08lx PRCR=0x%08lx\n",
        (unsigned long)DCMIPP->CMCR, (unsigned long)DCMIPP->PRCR);
  _info("dcmipp:   CMSR1=0x%08lx CMSR2=0x%08lx\n",
        (unsigned long)DCMIPP->CMSR1, (unsigned long)DCMIPP->CMSR2);
  _info("dcmipp:   CMIER=0x%08lx\n",
        (unsigned long)DCMIPP->CMIER);
  _info("dcmipp:   P1FSCR=0x%08lx P1FCTCR=0x%08lx\n",
        (unsigned long)DCMIPP->P1FSCR, (unsigned long)DCMIPP->P1FCTCR);
  _info("dcmipp:   P1PPCR=0x%08lx P1PPM0AR1=0x%08lx P1PPM0PR=0x%08lx\n",
        (unsigned long)DCMIPP->P1PPCR, (unsigned long)DCMIPP->P1PPM0AR1,
        (unsigned long)DCMIPP->P1PPM0PR);
  _info("dcmipp:   P1DSCR=0x%08lx P1DSSZR=0x%08lx P1DSRTIOR=0x%08lx\n",
        (unsigned long)DCMIPP->P1DSCR, (unsigned long)DCMIPP->P1DSSZR,
        (unsigned long)DCMIPP->P1DSRTIOR);

  /* Decode the CSI-2 error bits (SOT errors mean the PHY sees no/lossy
   * clock-data from the sensor)
   */

  if ((CSI->SR1 & (CSI_SR1_ESOTDL0F | CSI_SR1_ESOTDL1F)) != 0u)
    {
      _info("dcmipp:   !! SOT error on data lane (no/lossy CSI clock)\n");
    }

  if ((CSI->SR0 & (CSI_SR0_SYNCERRF | CSI_SR0_ECCERRF |
                   CSI_SR0_CRCERRF)) != 0u)
    {
      _info("dcmipp:   !! CSI sync/ECC/CRC error (corrupt data)\n");
    }

  if ((DCMIPP->CMSR2 & DCMIPP_CMSR2_P1FRAMEF) != 0u)
    {
      _info("dcmipp:   !! P1FRAMEF set (frame complete, ISR pending)\n");
    }

  /* Detailed decode of CSI SR0 / SR1 and PIPE1 status */

  _info("dcmipp:   SR0: SOF0F=%d EOF0F=%d SPKTF=%d VC0STATEF=%d"
        " CCFIFOFF=%d SYNCERR=%d ECC=%d CRC=%d IDERR=%d SPKTERR=%d\n",
        (CSI->SR0 & CSI_SR0_SOF0F) != 0, (CSI->SR0 & CSI_SR0_EOF0F) != 0,
        (CSI->SR0 & CSI_SR0_SPKTF) != 0,
        (CSI->SR0 & CSI_SR0_VC0STATEF) != 0,
        (CSI->SR0 & CSI_SR0_CCFIFOFF) != 0,
        (CSI->SR0 & CSI_SR0_SYNCERRF) != 0,
        (CSI->SR0 & CSI_SR0_ECCERRF) != 0,
        (CSI->SR0 & CSI_SR0_CRCERRF) != 0,
        (CSI->SR0 & CSI_SR0_IDERRF) != 0,
        (CSI->SR0 & CSI_SR0_SPKTERRF) != 0);
  _info("dcmipp:   SR1: CLKACT=%d | DL0:act=%d sync=%d stop=%d |"
        " DL1:act=%d sync=%d stop=%d\n",
        (CSI->SR1 & CSI_SR1_ACTCLF) != 0,
        (CSI->SR1 & CSI_SR1_ACTDL0F) != 0,
        (CSI->SR1 & CSI_SR1_SYNCDL0F) != 0,
        (CSI->SR1 & CSI_SR1_STOPDL0F) != 0,
        (CSI->SR1 & CSI_SR1_ACTDL1F) != 0,
        (CSI->SR1 & CSI_SR1_SYNCDL1F) != 0,
        (CSI->SR1 & CSI_SR1_STOPDL1F) != 0);
  _info("dcmipp:   PIPE1: CPTACT=%d P1FRAMEF=%d P1OVRF=%d\n",
        (DCMIPP->CMSR1 & DCMIPP_CMSR1_P1CPTACT) != 0,
        (DCMIPP->CMSR2 & DCMIPP_CMSR2_P1FRAMEF) != 0,
        (DCMIPP->CMSR2 & DCMIPP_CMSR2_P1OVRF) != 0);
}
