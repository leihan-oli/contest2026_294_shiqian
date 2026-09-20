/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_dma2d.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_DMA2D_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_DMA2D_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* DMA2D register offsets ***************************************************/

#define STM32_DMA2D_CR_OFFSET      0x0000  /* Control register */
#define STM32_DMA2D_ISR_OFFSET     0x0004  /* Interrupt status register */
#define STM32_DMA2D_IFCR_OFFSET    0x0008  /* Interrupt flag clear register */
#define STM32_DMA2D_OPFCCR_OFFSET  0x0034  /* Output PFC control register */
#define STM32_DMA2D_OCOLR_OFFSET   0x0038  /* Output color register */
#define STM32_DMA2D_OMAR_OFFSET    0x003c  /* Output memory address reg */
#define STM32_DMA2D_OOR_OFFSET     0x0040  /* Output offset register */
#define STM32_DMA2D_NLR_OFFSET     0x0044  /* Number of line register */

/* DMA2D register addresses *************************************************/

#define STM32_DMA2D_CR      (STM32_DMA2D_BASE + STM32_DMA2D_CR_OFFSET)
#define STM32_DMA2D_ISR     (STM32_DMA2D_BASE + STM32_DMA2D_ISR_OFFSET)
#define STM32_DMA2D_IFCR    (STM32_DMA2D_BASE + STM32_DMA2D_IFCR_OFFSET)
#define STM32_DMA2D_OPFCCR  (STM32_DMA2D_BASE + STM32_DMA2D_OPFCCR_OFFSET)
#define STM32_DMA2D_OCOLR   (STM32_DMA2D_BASE + STM32_DMA2D_OCOLR_OFFSET)
#define STM32_DMA2D_OMAR    (STM32_DMA2D_BASE + STM32_DMA2D_OMAR_OFFSET)
#define STM32_DMA2D_OOR     (STM32_DMA2D_BASE + STM32_DMA2D_OOR_OFFSET)
#define STM32_DMA2D_NLR     (STM32_DMA2D_BASE + STM32_DMA2D_NLR_OFFSET)

/* DMA2D CR register bits ***************************************************/

#define DMA2D_CR_START      (1 << 0)   /* Bit 0: Start transfer */
#define DMA2D_CR_MODE_SHIFT 16         /* Bits 16-18: Transfer mode */
#define DMA2D_CR_MODE_MASK  (0x7 << DMA2D_CR_MODE_SHIFT)
#  define DMA2D_CR_MODE_M2M      (0x0 << DMA2D_CR_MODE_SHIFT) /* mem-to-mem */
#  define DMA2D_CR_MODE_M2M_PFC  (0x1 << DMA2D_CR_MODE_SHIFT) /* + PFC */
#  define DMA2D_CR_MODE_M2M_BLEND (0x2 << DMA2D_CR_MODE_SHIFT)/* + blend */
#  define DMA2D_CR_MODE_R2M      (0x3 << DMA2D_CR_MODE_SHIFT) /* reg-to-mem */

/* DMA2D ISR register bits **************************************************/

#define DMA2D_ISR_TEIF      (1 << 0)   /* Transfer error interrupt flag */
#define DMA2D_ISR_TCIF      (1 << 1)   /* Transfer complete interrupt flag */
#define DMA2D_ISR_TWIF      (1 << 2)   /* Transfer watermark interrupt flag */
#define DMA2D_ISR_CAEIF     (1 << 3)   /* CLUT access error interrupt flag */
#define DMA2D_ISR_CTCIF     (1 << 4)   /* CLUT transfer complete flag */
#define DMA2D_ISR_CEIF      (1 << 5)   /* Configuration error flag */

/* DMA2D IFCR register bits (write-1-to-clear, same layout as ISR) **********/

#define DMA2D_IFCR_CTEIF    (1 << 0)   /* Clear transfer error flag */
#define DMA2D_IFCR_CTCIF    (1 << 1)   /* Clear transfer complete flag */
#define DMA2D_IFCR_CTWIF    (1 << 2)   /* Clear transfer watermark flag */
#define DMA2D_IFCR_CAECIF   (1 << 3)   /* Clear CLUT access error flag */
#define DMA2D_IFCR_CCTCIF   (1 << 4)   /* Clear CLUT transfer complete flag */
#define DMA2D_IFCR_CCEIF    (1 << 5)   /* Clear configuration error flag */

/* DMA2D OPFCCR output color-mode field (bits 2:0).  ARGB8888 == 0. */

#define DMA2D_OPFCCR_CM_SHIFT   0
#define DMA2D_OPFCCR_CM_MASK    (0x7 << DMA2D_OPFCCR_CM_SHIFT)
#  define DMA2D_OPFCCR_CM_ARGB8888 (0x0 << DMA2D_OPFCCR_CM_SHIFT)

/* DMA2D NLR register fields ************************************************/

#define DMA2D_NLR_NL_SHIFT  0          /* Bits 0-15: Number of lines */
#define DMA2D_NLR_NL_MASK   (0xffff << DMA2D_NLR_NL_SHIFT)
#define DMA2D_NLR_PL_SHIFT  16         /* Bits 16-29: Pixels per line */
#define DMA2D_NLR_PL_MASK   (0x3fff << DMA2D_NLR_PL_SHIFT)

/* RIFSC RIMC master-attribute registers ************************************
 *
 * RIF (Resource Isolation Framework): each bus master presents a compartment
 * ID (CID) on the bus; a RISAF region only accepts a transaction whose CID
 * is on that region's read/write whitelist.  RIMC_ATTRx[] assigns the CID
 * (and secure/priv attributes) a master presents.  Array base is at
 * RIFSC + 0xC10; DMA2D is master index 8, GPU2D index 7 (per ST HAL
 * stm32n6xx_hal_rif.h).  There is deliberately NO GPDMA1 index — GPDMA1's
 * CID is fixed by ROM/FSBL and cannot be reprogrammed here.
 */

#define STM32_RIFSC_RIMC_ATTR_OFFSET   0x0c10
#define STM32_RIFSC_RIMC_ATTR(n) \
  (STM32_RIFSC_BASE + STM32_RIFSC_RIMC_ATTR_OFFSET + ((n) << 2))

#define RIF_MASTER_INDEX_GPU2D  7
#define RIF_MASTER_INDEX_DMA2D  8
#define RIF_MASTER_INDEX_DCMIPP 9

#define RIFSC_RIMC_ATTR_MCID_SHIFT  4          /* Bits 4-6: master CID */
#define RIFSC_RIMC_ATTR_MCID_MASK   (0x7 << RIFSC_RIMC_ATTR_MCID_SHIFT)
#  define RIFSC_RIMC_ATTR_MCID(n)   ((n) << RIFSC_RIMC_ATTR_MCID_SHIFT)
#define RIFSC_RIMC_ATTR_MSEC        (1 << 8)   /* Master secure */
#define RIFSC_RIMC_ATTR_MPRIV       (1 << 9)   /* Master privileged */

/* RISAF region block (used read-only here, for diagnostics) ****************
 *
 * RISAF container: CR@0x00 (GLOCK bit0), then a REG[15] array starting at
 * +0x40, each region 0x40 bytes.  RISAF7 covers FLEXMEM where the DEV-boot
 * firmware runs (SRAM @ 0x34000400); reading REG[0].CIDCFGR tells us which
 * CID the region already grants read/write, i.e. the CID DMA2D must present.
 */

#define STM32_RISAF_CR_OFFSET       0x0000
#  define RISAF_CR_GLOCK            (1 << 0)   /* Global lock (frozen) */
#define STM32_RISAF_REG_BASE_OFFSET 0x0040
#define STM32_RISAF_REG_STRIDE      0x0040
#define STM32_RISAF_REG_CFGR        0x0000
#  define RISAF_REG_CFGR_BREN       (1 << 0)   /* Base region enable */
#  define RISAF_REG_CFGR_SEC        (1 << 8)   /* Region secure */
#define STM32_RISAF_REG_STARTR      0x0004
#define STM32_RISAF_REG_ENDR        0x0008
#define STM32_RISAF_REG_CIDCFGR     0x000c
#  define RISAF_REG_CIDCFGR_RDEN_SHIFT  0      /* Bits 0-7: read whitelist */
#  define RISAF_REG_CIDCFGR_WREN_SHIFT  16     /* Bits 16-23: write list */

#define STM32_RISAF_REG(base, n) \
  ((base) + STM32_RISAF_REG_BASE_OFFSET + ((n) * STM32_RISAF_REG_STRIDE))

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_DMA2D_H */
