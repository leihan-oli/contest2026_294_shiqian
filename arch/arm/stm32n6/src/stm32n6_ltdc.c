/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_ltdc.c
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
 * STM32N6 LTDC (LCD-TFT Display Controller) driver.
 *
 * Supports dual-layer overlay:
 *   Layer 0: Camera background (RGB565)
 *   Layer 1: Detection overlay (ARGB4444, transparent color keying)
 *
 * Reference: STM32N6 Getting Started ObjectDetection
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/video/fb.h>
#include <nuttx/kmalloc.h>
#include <nuttx/irq.h>
#include <syslog.h>
#include <string.h>
#include <assert.h>

#include "arm_internal.h"
#include "stm32n6_gpio.h"
#include "stm32n6_ltdc.h"
#include "hardware/stm32_ltdc.h"
#include "hardware/stm32_rcc.h"
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define LTDC_LAYER_BG     0
#define LTDC_LAYER_FG     1
#define LTDC_LAYER_COUNT  2

/* ATK-MD0700R-800480 (7" RGB LCD, 16-bit RGB565) timing.
 * Matches bare-metal rgblcd.c ID 0x7084:
 *   hsw=1 vsw=1 hbp=46 vbp=23 hfp=210 vfp=22, PCLK 33.33 MHz
 * Accumulated values (hsw+hbp-1 etc.) written directly to HW regs.
 */

#define LTDC_HW_HSW          1
#define LTDC_HW_VSW          1
#define LTDC_HW_HBP          46
#define LTDC_HW_VBP          23
#define LTDC_HW_HFP          210
#define LTDC_HW_VFP          22
#define LTDC_HW_ACTIVE_W     800
#define LTDC_HW_ACTIVE_H     480

#define LTDC_HW_AHBP         (LTDC_HW_HSW + LTDC_HW_HBP - 1)   /* 46 */
#define LTDC_HW_AVBP         (LTDC_HW_VSW + LTDC_HW_VBP - 1)   /* 23 */
#define LTDC_HW_AAW          (LTDC_HW_HSW + LTDC_HW_HBP + LTDC_HW_ACTIVE_W - 1)
#define LTDC_HW_AAH          (LTDC_HW_VSW + LTDC_HW_VBP + LTDC_HW_ACTIVE_H - 1)
#define LTDC_HW_TOTALW       (LTDC_HW_HSW + LTDC_HW_HBP + LTDC_HW_ACTIVE_W + \
                              LTDC_HW_HFP - 1)
#define LTDC_HW_TOTALH       (LTDC_HW_VSW + LTDC_HW_VBP + LTDC_HW_ACTIVE_H + \
                              LTDC_HW_VFP - 1)

/* LTDC pixel clock: PLL1(1200 MHz VCO, left by FSBL) / IC16 divider 36
 * = 33.33 MHz.  IC16CFGR.IC16INT is written as (divider - 1).
 */

#define LTDC_PCLK_DIVIDER    36
#define LTDC_PCLK_FREQ       1200000000ul / LTDC_PCLK_DIVIDER

/* RIF: LTDC1 RIMC master attributes (secure privileged, CID0).
 * RIMC_ATTR10 = RIFSC + 0xC10 + 10*4.  MSEC=bit8, MPRIV=bit9.
 */

#define STM32_RIFSC_RIMC_BASE (STM32_RIFSC_BASE + 0x0c10)
#define LTDC_RIMC_ATTR10      (STM32_RIFSC_RIMC_BASE + 10 * 4)
#define RIMC_ATTR_MSEC        (1 << 8)
#define RIMC_ATTR_MPRIV       (1 << 9)

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_ltdc_dev_s
{
  struct fb_vtable_s  vtable;     /* Framebuffer vtable (must be first) */
  sem_t               locksem;    /* Device lock */
  uint32_t            width;      /* Display width */
  uint32_t            height;     /* Display height */
  uint32_t            bg_fmt;     /* Background pixel format */
  uint32_t            fg_fmt;     /* Foreground pixel format */
  void               *bg_buffer;  /* Background framebuffer */
  void               *fg_buffer;  /* Foreground framebuffer */
  void               *fg_buffer2; /* Foreground second buffer */
  uint32_t            fg_idx;     /* Current foreground buffer index */
  uint32_t            fg_size;    /* Foreground buffer size */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32n6_ltdc_dev_s *g_ltdc_dev;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int ltdc_getvideoinfo(struct fb_vtable_s *vtable,
                              struct fb_videoinfo_s *vinfo)
{
  struct stm32n6_ltdc_dev_s *priv =
    (struct stm32n6_ltdc_dev_s *)vtable;

  vinfo->fmt = priv->bg_fmt;
  vinfo->xres = priv->width;
  vinfo->yres = priv->height;
  vinfo->nplanes = 1;

  return OK;
}

static int ltdc_getplaneinfo(struct fb_vtable_s *vtable,
                              int planeno,
                              struct fb_planeinfo_s *pinfo)
{
  struct stm32n6_ltdc_dev_s *priv =
    (struct stm32n6_ltdc_dev_s *)vtable;

  if (planeno != 0)
    {
      return -EINVAL;
    }

  pinfo->fbmem = priv->bg_buffer;
  pinfo->fblen = priv->width * priv->height * 2;
  pinfo->stride = priv->width * 2;
  pinfo->display = 0;
  pinfo->bpp = 16;

  return OK;
}

static int ltdc_open(struct fb_vtable_s *vtable)
{
  return OK;
}

static int ltdc_close(struct fb_vtable_s *vtable)
{
  return OK;
}

/****************************************************************************
 * Name: ltdc_clock_config
 *
 * Description:
 *   Enable LTDC (APB5) and configure the LTDC kernel clock (IC16).
 *   IC16 = PLL1 / 36 = 33.33 MHz (matches bare-metal 15_RGBLCD).
 *
 ****************************************************************************/

static void ltdc_clock_config(void)
{
  uint32_t regval;

  /* Enable APB5 (LTDC) clock via the SET alias (ENSR is write-1-to-set) */

  putreg32(RCC_APB5ENR_LTDCEN, STM32_RCC_APB5ENSR);

  /* ROOT CAUSE of "black screen when entering NSH" (2026-08-12, RM0486
   * 14.10.105): RCC_APB5LPENR.LTDCLPEN resets to 0, so during CPU Sleep
   * (the IDLE task runs WFI at the NSH prompt) the LTDC APB5 clock is
   * gated (EN AND LPEN = 0) and the LTDC stops -> screen goes black even
   * though all LTDC/RISAF/framebuffer state is correct.  Set LTDCLPEN via
   * the SET alias so the LTDC stays clocked in Sleep mode. */

  putreg32(RCC_APB5LPENR_LTDCLPEN, STM32_RCC_APB5LPENSR);

  /* ALSO keep the framebuffer SRAM (FLEXMEM + AXISRAM1/2) clocked during CPU
   * Sleep: RCC_MEMLPENR resets to 0, so during Sleep the SRAM is clock-gated
   * and the still-running LTDC cannot read the framebuffer -> FIFO underrun
   * -> the layer outputs DCCR (0 = black) -> screen goes black at NSH even
   * though LTDCLPEN keeps the LTDC module alive.  Set the memory sleep-enable
   * bits via the MEMLPENSR set alias. */

  putreg32(RCC_MEMLPENR_ALLAXISRAM, STM32_RCC_MEMLPENSR);

  /* IC16: source = PLL1 (IC16SEL=0), divider = 36 (IC16INT=35) */

  regval = ((LTDC_PCLK_DIVIDER - 1) << RCC_IC16CFGR_IC16INT_SHIFT) |
           (RCC_IC16CFGR_IC16SEL_PLL1 << RCC_IC16CFGR_IC16SEL_SHIFT);
  putreg32(regval, STM32_RCC_IC16CFGR);

  /* Enable the IC16 divider */

  putreg32(RCC_DIVENR_IC16EN, STM32_RCC_DIVENSR);

  /* Select IC16 as the LTDC kernel clock source (CCIPR4.LTDCSEL = 0b10).
   * Without this the LTDC has no pixel clock even though IC16 is enabled,
   * so CDSR stays 0 and the display stays black.  Matches bare-metal
   * HAL_RCCEx_PeriphCLKConfig (LtdcClockSelection = RCC_LTDCCLKSOURCE_IC16). */

  modifyreg32(STM32_RCC_CCIPR4, RCC_CCIPR4_LTDCSEL_MASK,
              RCC_CCIPR4_LTDCSEL_IC16);

}

/****************************************************************************
 * Name: ltdc_rif_config
 *
 * Description:
 *   Grant LTDC1 read access to SRAM through the RIF (RIMC master
 *   attributes).  The custom FSBL grants DMA2D/NPU/GPDMA1 but not LTDC1,
 *   so without this the LTDC DMA reads from internal SRAM are filtered
 *   by RISAF and the display stays blank.
 *
 ****************************************************************************/

static void ltdc_rif_config(void)
{
  /* Ensure the RIFSC clock is on (FSBL already enables it; be safe) */

  putreg32(RCC_AHB3ENR_RIFSCEN, STM32_RCC_AHB3ENSR);

  /* LTDC1: CID0, secure privileged (MSEC|MPRIV) */

  putreg32(RIMC_ATTR_MSEC | RIMC_ATTR_MPRIV, LTDC_RIMC_ATTR10);

  /* LTDCL1 (LTDC layer-1 data path) slave attributes = SEC|PRIV.
   * RISC_SECCFGRx[3] bit7 (secure) + RISC_PRIVCFGRx[3] bit7 (privileged),
   * RIFSC base = 0x54024000 (secure alias).  Bare-metal
   * SystemIsolation_Config() sets these; without them the layer-1
   * framebuffer reads are filtered by RIF and the panel stays blank
   * even though LTDCEN/LEN are set. */

  modifyreg32(0x54024000UL + 0x01c, 0, (1 << 7));  /* RISC_SECCFGR3.SEC7 */
  modifyreg32(0x54024000UL + 0x03c, 0, (1 << 7));  /* RISC_PRIVCFGR3.PRIV7 */

  /* Open the framebuffer SRAM windows for the LTDC (CID0) reads.
   * RM0486 Table 24: RISAF2 = CPU AXI RAM0 (AXISRAM1, 1MB @0x54027000),
   * RISAF3 = CPU AXI RAM1 (AXISRAM2, 1MB @0x54028000), RISAF7 = FLEXMEM
   * (512KB @0x5402C000, already opened by the FSBL).  The 800x480 RGB565
   * framebuffer (768000B) spans FLEXMEM + AXISRAM1 + AXISRAM2; the FSBL only
   * opened RISAF7, so the LTDC's CID0 reads past 0x3407FFFF are denied by the
   * default region rule (secure+privileged+CID1 only) -> layer FIFO underrun
   * -> only the top lines display.  Open RISAF2+RISAF3 REG0 for every CID.
   * STARTR/ENDR are byte offsets from each protected space base; a 1MB space
   * accepts offsets 0x00000-0xFFFFF.  Order: clear GLOCK, disable, write
   * STARTR/ENDR/CIDCFGR, enable LAST. */

  putreg32(0, 0x5402c000UL);       /* RISAF7 CR = 0 (clear GLOCK) */
  putreg32(0, 0x5402c080UL + 0x00);  /* RISAF7 REG1.CFGR = 0 (disabled) */
  putreg32(0, 0x5402c0c0UL + 0x00);  /* RISAF7 REG2.CFGR = 0 (disabled) */
  putreg32(0, 0x54027000UL);       /* RISAF2 CR = 0 (clear GLOCK) */
  putreg32(0, 0x54027040UL + 0x00);  /* RISAF2 REG0.CFGR = 0 (disable first) */
  putreg32(0x00000000, 0x54027040UL + 0x04);  /* REG0.STARTR = 0 (AXISRAM1) */
  putreg32(0x000fffff, 0x54027040UL + 0x08);  /* REG0.ENDR = 0xFFFFF (1MB) */
  putreg32(0x00ff00ff, 0x54027040UL + 0x0c);  /* REG0.CIDCFGR (all CID R/W) */
  putreg32(0x00ff0101, 0x54027040UL + 0x00);  /* REG0.CFGR (enable LAST) */
  putreg32(0, 0x54028000UL);       /* RISAF3 CR = 0 (clear GLOCK) */
  putreg32(0, 0x54028040UL + 0x00);  /* RISAF3 REG0.CFGR = 0 (disable first) */
  putreg32(0x00000000, 0x54028040UL + 0x04);  /* REG0.STARTR = 0 (AXISRAM2) */
  putreg32(0x000fffff, 0x54028040UL + 0x08);  /* REG0.ENDR = 0xFFFFF (1MB) */
  putreg32(0x00ff00ff, 0x54028040UL + 0x0c);  /* REG0.CIDCFGR (all CID R/W) */
  putreg32(0x00ff0101, 0x54028040UL + 0x00);  /* REG0.CFGR (enable LAST) */
}

/****************************************************************************
 * Name: ltdc_gpio_config
 *
 * Description:
 *   Configure the 16-bit RGB565 LTDC interface (AF14), sync/clock lines
 *   and the backlight (PA3, active high).
 *
 ****************************************************************************/

static void ltdc_gpio_config(void)
{
  stm32n6_configgpio(GPIO_LTDC_R3);
  stm32n6_configgpio(GPIO_LTDC_R4);
  stm32n6_configgpio(GPIO_LTDC_R5);
  stm32n6_configgpio(GPIO_LTDC_R6);
  stm32n6_configgpio(GPIO_LTDC_R7);
  stm32n6_configgpio(GPIO_LTDC_G2);
  stm32n6_configgpio(GPIO_LTDC_G3);
  stm32n6_configgpio(GPIO_LTDC_G4);
  stm32n6_configgpio(GPIO_LTDC_G5);
  stm32n6_configgpio(GPIO_LTDC_G6);
  stm32n6_configgpio(GPIO_LTDC_G7);
  stm32n6_configgpio(GPIO_LTDC_B3);
  stm32n6_configgpio(GPIO_LTDC_B4);
  stm32n6_configgpio(GPIO_LTDC_B5);
  stm32n6_configgpio(GPIO_LTDC_B6);
  stm32n6_configgpio(GPIO_LTDC_B7);
  stm32n6_configgpio(GPIO_LTDC_CLK);
  stm32n6_configgpio(GPIO_LTDC_HSYNC);
  stm32n6_configgpio(GPIO_LTDC_VSYNC);
  stm32n6_configgpio(GPIO_LTDC_DE);

  /* Backlight on (PA3 output high) */

  stm32n6_configgpio(GPIO_LTDC_BL);
  stm32n6_gpiowrite(GPIO_LTDC_BL, true);
}

/****************************************************************************
 * Name: ltdc_hw_init
 *
 * Description:
 *   Program the LTDC registers for the 800x480 RGB565 panel and enable
 *   Layer 1 (the /dev/fb0 framebuffer).
 *
 ****************************************************************************/

static void ltdc_hw_init(void *fb)
{
  uint32_t regval;
  uint32_t pitch;

  /* Synchronization / timing registers (ATK-MD0700R-800480) */

  putreg32((LTDC_HW_HSW - 1) << LTDC_SSCR_HSW_SHIFT |
           (LTDC_HW_VSW - 1) << LTDC_SSCR_VSH_SHIFT,
           STM32_LTDC_SSCR);
  putreg32((LTDC_HW_AHBP << LTDC_BPCR_AHBP_SHIFT) |
           (LTDC_HW_AVBP << LTDC_BPCR_AVBP_SHIFT),
           STM32_LTDC_BPCR);
  putreg32((LTDC_HW_AAW << LTDC_AWCR_AAW_SHIFT) |
           (LTDC_HW_AAH << LTDC_AWCR_AAH_SHIFT),
           STM32_LTDC_AWCR);
  putreg32((LTDC_HW_TOTALW << LTDC_TWCR_TOTALW_SHIFT) |
           (LTDC_HW_TOTALH << LTDC_TWCR_TOTALH_SHIFT),
           STM32_LTDC_TWCR);

  /* Global control: HS/VS/DE active low (pol=0), PCLK normal (pol=0).
   * LTDC stays disabled until the layer is configured. */

  putreg32(0, STM32_LTDC_GCR);

  /* Background color = black (RGB565) */

  putreg32(0, STM32_LTDC_BCCR);

  /* Layer 1 (the /dev/fb0 framebuffer) */

  putreg32(0, STM32_LTDC_L1CR);               /* disable while configuring */

  /* STM32N6 window coordinates INCLUDE the sync+back-porch offset (the
   * HAL writes WHSTPOS = WindowX0 + AHBP + 1, WHSPPOS = WindowX1 + AHBP).
   * With AHBP=46/AVBP=23 this puts the window exactly on the active area:
   *   WHPCR = (0+46+1) | ((800+46)<<16) = 0x034E002F
   *   WVPCR = (0+23+1) | ((480+23)<<16) = 0x01F70018 */
  regval = ((LTDC_HW_AHBP + 1) << LTDC_LxWHPCR_WHSTPOS_SHIFT) |
           ((LTDC_HW_AHBP + LTDC_HW_ACTIVE_W) << LTDC_LxWHPCR_WHSPPOS_SHIFT);
  putreg32(regval, STM32_LTDC_L1WHPCR);

  regval = ((LTDC_HW_AVBP + 1) << LTDC_LxWVPCR_WVSTPOS_SHIFT) |
           ((LTDC_HW_AVBP + LTDC_HW_ACTIVE_H) << LTDC_LxWVPCR_WVSPPOS_SHIFT);
  putreg32(regval, STM32_LTDC_L1WVPCR);

  putreg32(LTDC_PF_RGB565, STM32_LTDC_L1PFCR);   /* RGB565 */

  pitch = LTDC_HW_ACTIVE_W * 2;                  /* 1600 bytes */
  /* STM32N6 CFBLL = line length = width*bpp + 7 (NOT -1 like F4/H7);
   * CFBLNR = number of lines (NOT lines-1).  Both verified against the
   * bare-metal HAL_LTDC_ConfigLayer write pattern. */
  regval = ((pitch + 7) << LTDC_LxCFBLR_CFBLL_SHIFT) |
           (pitch << LTDC_LxCFBLR_CFBPITCH_SHIFT);
  putreg32(regval, STM32_LTDC_L1CFBLR);

  putreg32(LTDC_HW_ACTIVE_H, STM32_LTDC_L1CFBLNR);
  putreg32((uint32_t)fb, STM32_LTDC_L1CFBAR);

  /* Layer blending + enable (matches bare-metal 15_RGBLCD: Alpha=255,
   * BlendingFactor1/2=CA, RGB565), then enable LTDC and reload shadow
   * registers. */

  putreg32(0, STM32_LTDC_L1DCCR);  /* default color = black */
  putreg32(LTDC_LxCACR_CONSTA(255), STM32_LTDC_L1CACR);
  putreg32(LTDC_LxBFCR_BF1_CA | LTDC_LxBFCR_BF2_1MCA, STM32_LTDC_L1BFCR);
  /* Enable layer: LEN only.  NOTE: on STM32N6 LxCR.CKEN (bit1) is the COLOR
   * KEYING enable, NOT a layer clock gate - enabling it with CKCR=0 would
   * key out black pixels.  Bare-metal HAL uses LEN only. */
  putreg32(LTDC_LxCR_LEN, STM32_LTDC_L1CR);
  modifyreg32(STM32_LTDC_GCR, 0, LTDC_GCR_LTDCEN);
  /* Reload layer 1 shadow registers explicitly (LxRCR.IMR).  Bare-metal
   * HAL_LTDC_ConfigLayer writes LxRCR = IMR|GRMSK; without this the layer
   * configuration (CFBAR, window, pixel format, ...) never reaches the
   * active engine and the framebuffer never appears. */
  putreg32(LTDC_LxRCR_IMR | LTDC_LxRCR_GRMSK, STM32_LTDC_L1RCR);
  modifyreg32(STM32_LTDC_SRCR, 0, LTDC_SRCR_IMR);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/



/****************************************************************************
 * Name: stm32n6_ltdc_init
 *
 * Description:
 *   Initialize LTDC display controller.
 *   Registers /dev/fb0 framebuffer device.
 *
 *   Configuration:
 *     - Layer 0 (background): Camera feed, RGB565, continuous update
 *     - Layer 1 (foreground): Detection overlay, ARGB4444, transparent
 *     - Color keying: 0x000000 (black = transparent)
 *     - Double-buffered foreground for tear-free overlay updates
 *
 * Input Parameters:
 *   width   - Display width (e.g. 800)
 *   height  - Display height (e.g. 480)
 *   bg_buf  - Background framebuffer (camera output)
 *   fg_buf1 - Foreground framebuffer 1 (overlay)
 *   fg_buf2 - Foreground framebuffer 2 (overlay, double-buffer)
 *
 * Returned Value:
 *   OK on success, negated errno on failure.
 *
 ****************************************************************************/

int stm32n6_ltdc_init(uint32_t width, uint32_t height,
                        void *bg_buf, void *fg_buf1,
                        void *fg_buf2)
{
  struct stm32n6_ltdc_dev_s *priv;
  int ret;

  /* Allocate device structure */

  priv = kmm_zalloc(sizeof(struct stm32n6_ltdc_dev_s));
  if (priv == NULL)
    {
      syslog(LOG_ERR, "ltdc: out of memory\n");
      return -ENOMEM;
    }

  priv->width = width;
  priv->height = height;
  priv->bg_fmt = FB_FMT_RGB16_565;
  priv->fg_fmt = FB_FMT_RGB16_565;  /* ARGB4444 packed as 16-bit */
  priv->bg_buffer = bg_buf;
  priv->fg_buffer = fg_buf1;
  priv->fg_buffer2 = fg_buf2;
  priv->fg_idx = 0;
  priv->fg_size = width * height * 2;

  nxsem_init(&priv->locksem, 0, 1);

  /* Initialize vtable */

  priv->vtable.getvideoinfo = ltdc_getvideoinfo;
  priv->vtable.getplaneinfo = ltdc_getplaneinfo;
  priv->vtable.open = ltdc_open;
  priv->vtable.close = ltdc_close;

  /* Register framebuffer device (openvela/NuttX fb framework) */

  ret = fb_register_device(0, 0, &priv->vtable);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ltdc: register failed: %d\n", ret);
      nxsem_destroy(&priv->locksem);
      kmm_free(priv);
      return ret;
    }

  g_ltdc_dev = priv;

  syslog(LOG_INFO, "ltdc: registered /dev/fb0 (%lux%lu "
         "RGB565+ARGB4444)\n",
         (unsigned long)width, (unsigned long)height);

  /* Program the LTDC hardware (clock, RIF, GPIO, timing, layer) */

  ltdc_clock_config();
  ltdc_rif_config();
  ltdc_gpio_config();
  ltdc_hw_init(priv->bg_buffer);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_ltdc_set_bg_buffer
 *
 * Description:
 *   Update background layer buffer address.
 *   Used to swap camera frame buffers.
 *
 ****************************************************************************/

int stm32n6_ltdc_set_bg_buffer(void *buffer)
{
  struct stm32n6_ltdc_dev_s *priv = g_ltdc_dev;

  if (priv == NULL)
    {
      return -ENODEV;
    }

  priv->bg_buffer = buffer;

  /* TODO: Update LTDC Layer 0 framebuffer address
   * HAL_LTDC_SetAddress_NoReload(&hltdc, (uint32_t)buffer, LTDC_LAYER_BG);
   * HAL_LTDC_ReloadLayer(&hltdc, LTDC_RELOAD_IMMEDIATE, LTDC_LAYER_BG);
   */

  return OK;
}

/****************************************************************************
 * Name: stm32n6_ltdc_swap_fg_buffer
 *
 * Description:
 *   Swap foreground double buffer for tear-free overlay update.
 *   Returns pointer to the buffer that is now safe to write.
 *
 ****************************************************************************/

void *stm32n6_ltdc_swap_fg_buffer(void)
{
  struct stm32n6_ltdc_dev_s *priv = g_ltdc_dev;
  void *safe_buffer;

  if (priv == NULL)
    {
      return NULL;
    }

  /* Swap buffers */

  if (priv->fg_idx == 0)
    {
      priv->fg_idx = 1;
      safe_buffer = priv->fg_buffer2;
    }
  else
    {
      priv->fg_idx = 0;
      safe_buffer = priv->fg_buffer;
    }

  /* TODO: Update LTDC Layer 1 framebuffer address (disabled while the
   * framebuffer lives in a single RISAF-authorized buffer; double
   * buffering needs an extra 768 KB of authorized SRAM). */

  return safe_buffer;
}

/****************************************************************************
 * Name: stm32n6_ltdc_fill_fg_rect
 *
 * Description:
 *   Fill a rectangle on the foreground layer.
 *   Used for drawing bounding boxes and stats panel.
 *
 ****************************************************************************/

int stm32n6_ltdc_fill_fg_rect(uint32_t x, uint32_t y,
                                uint32_t w, uint32_t h,
                                uint16_t color)
{
  struct stm32n6_ltdc_dev_s *priv = g_ltdc_dev;
  uint16_t *buf;
  uint32_t row;
  uint32_t col;

  if (priv == NULL)
    {
      return -ENODEV;
    }

  /* Clamp to screen bounds */

  if (x >= priv->width || y >= priv->height)
    {
      return -EINVAL;
    }

  if (x + w > priv->width)
    {
      w = priv->width - x;
    }

  if (y + h > priv->height)
    {
      h = priv->height - y;
    }

  /* Get current foreground buffer (the one being displayed) */

  buf = (priv->fg_idx == 0) ?
        (uint16_t *)priv->fg_buffer :
        (uint16_t *)priv->fg_buffer2;

  /* Fill rectangle */

  for (row = 0; row < h; row++)
    {
      uint16_t *line = &buf[(y + row) * priv->width + x];
      for (col = 0; col < w; col++)
        {
          line[col] = color;
        }
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_ltdc_clear_fg
 *
 * Description:
 *   Clear foreground layer to transparent (0x0000).
 *
 ****************************************************************************/

void stm32n6_ltdc_clear_fg(void)
{
  struct stm32n6_ltdc_dev_s *priv = g_ltdc_dev;

  if (priv == NULL)
    {
      return;
    }

  /* Get the safe buffer (not currently displayed) */

  void *buf = (priv->fg_idx == 0) ?
              priv->fg_buffer2 : priv->fg_buffer;

  memset(buf, 0, priv->fg_size);
}

/****************************************************************************
 * Name: stm32n6_ltdc_get_fg_buffer
 *
 * Description:
 *   Get pointer to the safe-to-write foreground buffer.
 *
 ****************************************************************************/

void *stm32n6_ltdc_get_fg_buffer(void)
{
  struct stm32n6_ltdc_dev_s *priv = g_ltdc_dev;

  if (priv == NULL)
    {
      return NULL;
    }

  return (priv->fg_idx == 0) ?
         priv->fg_buffer2 : priv->fg_buffer;
}
