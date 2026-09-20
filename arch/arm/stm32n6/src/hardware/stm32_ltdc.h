/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_ltdc.h
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
 * STM32N6 LTDC (LCD-TFT Display Controller) register definitions.
 *
 * Register offsets verified against STM32N647xx (RM0486):
 *   global regs @ +0x08..0x90, Layer1 @ +0x100, Layer2 @ +0x200
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_LTDC_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_LTDC_H

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* LTDC is on APB5 (secure alias at 0x58001000) */

#define STM32_LTDC_BASE         0x58001000ul

/* Global registers */

#define STM32_LTDC_SSCR         (STM32_LTDC_BASE + 0x08)   /* Sync size */
#define STM32_LTDC_BPCR         (STM32_LTDC_BASE + 0x0c)   /* Back porch */
#define STM32_LTDC_AWCR         (STM32_LTDC_BASE + 0x10)   /* Active width */
#define STM32_LTDC_TWCR         (STM32_LTDC_BASE + 0x14)   /* Total width */
#define STM32_LTDC_ISR          (STM32_LTDC_BASE + 0x38)   /* Interrupt status */
#define STM32_LTDC_CDSR         (STM32_LTDC_BASE + 0x48)   /* Display status */
#define STM32_LTDC_CPSR         (STM32_LTDC_BASE + 0x44)   /* Current pixel pos (X@16, Y@0) */
#define STM32_LTDC_GCR          (STM32_LTDC_BASE + 0x18)   /* Global control */
#define STM32_LTDC_SRCR         (STM32_LTDC_BASE + 0x24)   /* Shadow reload */
#define STM32_LTDC_BCCR         (STM32_LTDC_BASE + 0x2c)   /* Background color */

/* Layer1 (0x100) / Layer2 (0x200) */

#define STM32_LTDC_L1RCR        (STM32_LTDC_BASE + 0x108)
#define STM32_LTDC_L1CR         (STM32_LTDC_BASE + 0x10c)

/* LxRCR: layer reload control (IMR = immediate reload, GRMSK = mask the
 * global SRCR-driven reload so the layer is reloaded explicitly). */

#define LTDC_LxRCR_IMR          (1 << 0)
#define LTDC_LxRCR_GRMSK        (1 << 2)
#define STM32_LTDC_L1WHPCR      (STM32_LTDC_BASE + 0x110)
#define STM32_LTDC_L1WVPCR      (STM32_LTDC_BASE + 0x114)
#define STM32_LTDC_L1PFCR       (STM32_LTDC_BASE + 0x11c)
#define STM32_LTDC_L1CFBAR      (STM32_LTDC_BASE + 0x134)
#define STM32_LTDC_L1CFBLR      (STM32_LTDC_BASE + 0x138)
#define STM32_LTDC_L1CFBLNR     (STM32_LTDC_BASE + 0x13c)

/* Additional STM32N6 layer/global registers (diagnostics; default values) */

#define STM32_LTDC_L1C0R        (STM32_LTDC_BASE + 0x100) /* capability 0 */
#define STM32_LTDC_L1C1R        (STM32_LTDC_BASE + 0x104) /* capability 1 */
#define STM32_LTDC_L1DCCR       (STM32_LTDC_BASE + 0x124) /* default color */
#define STM32_LTDC_L1BLCR       (STM32_LTDC_BASE + 0x12c) /* burst length */
#define STM32_LTDC_L1PCR        (STM32_LTDC_BASE + 0x130) /* planar config */
#define STM32_LTDC_FUTR         (STM32_LTDC_BASE + 0x90)  /* FIFO underrun thr */

/* SSCR bit definitions */

#define LTDC_SSCR_VSH_SHIFT     (0)
#define LTDC_SSCR_VSH_MASK      (0xfff << LTDC_SSCR_VSH_SHIFT)
#define LTDC_SSCR_HSW_SHIFT     (16)
#define LTDC_SSCR_HSW_MASK      (0xfff << LTDC_SSCR_HSW_SHIFT)

/* BPCR bit definitions */

#define LTDC_BPCR_AVBP_SHIFT    (0)
#define LTDC_BPCR_AVBP_MASK     (0xfff << LTDC_BPCR_AVBP_SHIFT)
#define LTDC_BPCR_AHBP_SHIFT    (16)
#define LTDC_BPCR_AHBP_MASK     (0xfff << LTDC_BPCR_AHBP_SHIFT)

/* AWCR bit definitions */

#define LTDC_AWCR_AAH_SHIFT     (0)
#define LTDC_AWCR_AAH_MASK      (0xfff << LTDC_AWCR_AAH_SHIFT)
#define LTDC_AWCR_AAW_SHIFT     (16)
#define LTDC_AWCR_AAW_MASK      (0xfff << LTDC_AWCR_AAW_SHIFT)

/* TWCR bit definitions */

#define LTDC_TWCR_TOTALH_SHIFT  (0)
#define LTDC_TWCR_TOTALH_MASK   (0xfff << LTDC_TWCR_TOTALH_SHIFT)
#define LTDC_TWCR_TOTALW_SHIFT  (16)
#define LTDC_TWCR_TOTALW_MASK   (0xfff << LTDC_TWCR_TOTALW_SHIFT)

/* GCR bit definitions (STM32N6: GCR @ +0x18) */

#define LTDC_GCR_LTDCEN         (1 << 0)  /* LTDC enable */
#define LTDC_GCR_BCKEN          (1 << 17) /* Background (BCCR) layer enable */
#define LTDC_GCR_PCPOL          (1 << 28) /* Input pixel clock polarity */
#define LTDC_GCR_DEPOL          (1 << 29) /* Data enable polarity */
#define LTDC_GCR_VSPOL          (1 << 30) /* Vertical sync polarity */
#define LTDC_GCR_HSPOL          (1 << 31) /* Horizontal sync polarity */

/* SRCR bit definitions */

#define LTDC_SRCR_IMR           (1 << 0)  /* Immediate reload */
#define LTDC_SRCR_VBR           (1 << 1)  /* Vertical blanking reload */

/* Layer CR bit definitions (STM32N6: LEN=bit0, CKEN=bit1, VPDEN=bit2,
 * CLUTEN=bit4, HMEN=bit8, DCBEN=bit9; there is NO BLEN/COLKEN field -
 * blending factors live in LxBFCR). */

#define LTDC_LxCR_LEN           (1 << 0)  /* Layer enable */
#define LTDC_LxCR_CKEN          (1 << 1)  /* Color keying enable (NOT a clock gate) */
#define LTDC_LxCR_VPDEN         (1 << 2)  /* Visual panel data enable */
#define LTDC_LxCR_CLUTEN        (1 << 4)  /* CLUT enable */
#define LTDC_LxCR_HMEN          (1 << 8)  /* Horizontal mirror enable */
#define LTDC_LxCR_DCBEN         (1 << 9)  /* Dither color bar enable */

/* Layer constant-alpha (CACR): STM32N6 keeps CONSTA in a dedicated CACR
 * register (unlike F4/H7 which pack it into LxCR bits 24-31).  CACR = 0
 * makes the layer fully transparent, so only the background shows. */

#define STM32_LTDC_L1CACR       (STM32_LTDC_BASE + 0x120)
#define LTDC_LxCACR_CONSTA_SHIFT (0)
#define LTDC_LxCACR_CONSTA_MASK  (0xff << LTDC_LxCACR_CONSTA_SHIFT)
#define LTDC_LxCACR_CONSTA(n)    (((n) & 0xff) << LTDC_LxCACR_CONSTA_SHIFT)

/* Layer blending-factors (LxBFCR @ +0x128): BF2=bits0-2, BF1=bits8-10.
 * Bare-metal 15_RGBLCD uses BF1=constant alpha (0b100), BF2=1-CA (0b101),
 * giving out = CA*src + (1-CA)*dst.  With CACR=255 (CA=1) the layer
 * renders the framebuffer opaque. */

#define STM32_LTDC_L1BFCR       (STM32_LTDC_BASE + 0x128)
#define LTDC_LxBFCR_BF2_SHIFT   (0)
#define LTDC_LxBFCR_BF2_MASK    (0x7 << LTDC_LxBFCR_BF2_SHIFT)
#define LTDC_LxBFCR_BF2_1MCA    (0x5 << LTDC_LxBFCR_BF2_SHIFT) /* 1 - CA */
#define LTDC_LxBFCR_BF1_SHIFT   (8)
#define LTDC_LxBFCR_BF1_MASK    (0x7 << LTDC_LxBFCR_BF1_SHIFT)
#define LTDC_LxBFCR_BF1_CA      (0x4 << LTDC_LxBFCR_BF1_SHIFT) /* CA */

/* Layer WHPCR / WVPCR (window start/stop, 12-bit each) */

#define LTDC_LxWHPCR_WHSTPOS_SHIFT (0)
#define LTDC_LxWHPCR_WHSTPOS_MASK  (0xfff << LTDC_LxWHPCR_WHSTPOS_SHIFT)
#define LTDC_LxWHPCR_WHSPPOS_SHIFT (16)
#define LTDC_LxWHPCR_WHSPPOS_MASK  (0xfff << LTDC_LxWHPCR_WHSPPOS_SHIFT)

#define LTDC_LxWVPCR_WVSTPOS_SHIFT (0)
#define LTDC_LxWVPCR_WVSTPOS_MASK  (0xfff << LTDC_LxWVPCR_WVSTPOS_SHIFT)
#define LTDC_LxWVPCR_WVSPPOS_SHIFT (16)
#define LTDC_LxWVPCR_WVSPPOS_MASK  (0xfff << LTDC_LxWVPCR_WVSPPOS_SHIFT)

/* Layer PFCR: pixel format (RGB565 = 4 on STM32N6) */

#define LTDC_LxPFCR_PF_SHIFT    (0)
#define LTDC_LxPFCR_PF_MASK     (0x7 << LTDC_LxPFCR_PF_SHIFT)
#define LTDC_PF_ARGB8888        (0)
#define LTDC_PF_RGB888          (1)
#define LTDC_PF_RGB565          (4)

/* Layer CFBAR: color frame buffer address (32-bit) */

#define LTDC_LxCFBAR_CFBADD     (0xffffffff)

/* Layer CFBLR: line length (CFBLL bits 0-12) + pitch (bits 16-28) */

#define LTDC_LxCFBLR_CFBLL_SHIFT   (0)
#define LTDC_LxCFBLR_CFBLL_MASK    (0x1fff << LTDC_LxCFBLR_CFBLL_SHIFT)
#define LTDC_LxCFBLR_CFBPITCH_SHIFT (16)
#define LTDC_LxCFBLR_CFBPITCH_MASK (0x1fff << LTDC_LxCFBLR_CFBPITCH_SHIFT)

/* Layer CFBLNR: line number (bits 0-10) */

#define LTDC_LxCFBLNR_CFBLNBR_SHIFT (0)
#define LTDC_LxCFBLNR_CFBLNBR_MASK  (0x7ff << LTDC_LxCFBLNR_CFBLNBR_SHIFT)

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_LTDC_H */
