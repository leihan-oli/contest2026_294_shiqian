/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_dcmipp.h
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
 * Port of the ST HAL_DCMIPP register sequences (STM32Cube_FW_N6) into the
 * NuttX arch layer, covering the subset used by the IMX335 sensor on the
 * ALIENTEK STM32N647 board:
 *   CSI-2: 2 data lanes, PHY 1600 Mbps, VC0 RAW10
 *   PIPE1: RAW10 -> RGB565 800x480 (hardware ISP demosaic), continuous
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_DCMIPP_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_DCMIPP_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

#include "hardware/stm32_dcmipp.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* DCMIPP capture modes (P1FCTCR.CPTMODE) */

#define DCMIPP_MODE_CONTINUOUS   (0)
#define DCMIPP_MODE_SNAPSHOT     DCMIPP_P1FCTCR_CPTMODE

/* DCMIPP pipe numbers */

#define DCMIPP_PIPE0             (0)
#define DCMIPP_PIPE1             (1)
#define DCMIPP_PIPE2             (2)

/* DCMIPP CSI virtual channels */

#define DCMIPP_VIRTUAL_CHANNEL0  (0)
#define DCMIPP_VIRTUAL_CHANNEL1  (1)
#define DCMIPP_VIRTUAL_CHANNEL2  (2)
#define DCMIPP_VIRTUAL_CHANNEL3  (3)

/* DCMIPP CSI configuration values used by the IMX335 driver */

#define DCMIPP_CSI_ONE_DATA_LANES (1u << CSI_LMCFGR_LANENB_Pos)
#define DCMIPP_CSI_TWO_DATA_LANES (2u << CSI_LMCFGR_LANENB_Pos)
#define DCMIPP_CSI_PHYSICAL_DATA_LANES  1u
#define DCMIPP_CSI_PHY_BT_1600          44u   /* HAL_CSI_BT_1600 */
#define DCMIPP_CSI_PHY_BT_800           28u   /* HAL_CSI_BT_800 */
#define DCMIPP_CSI_PHY_BT_1100          34u   /* HAL_CSI_BT_1100 */
#define DCMIPP_CSI_PHY_BT_1150          35u   /* HAL_CSI_BT_1150 */
#define DCMIPP_CSI_PHY_BT_1200          36u   /* HAL_CSI_BT_1200 */

#define DCMIPP_CSI_DATA_LANE0     1u
#define DCMIPP_CSI_DATA_LANE1     2u

#define DCMIPP_CSI_DT_BPP10      3u     /* VC data type format: 10-bit (ST HAL value) */
#define DCMIPP_DT_RAW10          0x2bu  /* CSI data type RAW10 */

#define DCMIPP_DTMODE_DTIDA      (0)
#define DCMIPP_DTMODE_DTIDA_OR_DTIDB (1u << DCMIPP_P0FSCR_DTMODE_Pos)
#define DCMIPP_DTMODE_ALL         (3u << DCMIPP_P0FSCR_DTMODE_Pos)

#define DCMIPP_FRAME_RATE_ALL    (0)
#define DCMIPP_PIXEL_PACKER_FORMAT_RGB565_1 (0x01u)

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* DCMIPP CSI configuration */

struct dcmipp_csi_config_s
{
  uint32_t num_lanes;        /* DCMIPP_CSI_ONE/TWO_DATA_LANES */
  uint32_t lane_mapping;     /* DCMIPP_CSI_PHYSICAL_DATA_LANES */
  uint32_t phy_bitrate;      /* DCMIPP_CSI_PHY_BT_xxx */
};

/* DCMIPP CSI pipe (flow) configuration */

struct dcmipp_csi_pipe_config_s
{
  uint32_t datatype_mode;    /* DCMIPP_DTMODE_xxx */
  uint32_t datatype_ida;     /* DCMIPP_DT_xxx */
  uint32_t datatype_idb;     /* DCMIPP_DT_xxx */
};

/* DCMIPP pipe configuration */

struct dcmipp_pipe_config_s
{
  uint32_t frame_rate;       /* DCMIPP_FRAME_RATE_xxx */
  uint32_t pixel_pitch;      /* bytes per line */
  uint32_t pixel_format;     /* DCMIPP_PIXEL_PACKER_FORMAT_xxx */
};

/* DCMIPP downsize configuration */

struct dcmipp_downsize_config_s
{
  uint32_t vsize;
  uint32_t hsize;
  uint32_t vratio;
  uint32_t hratio;
  uint32_t vdivfactor;
  uint32_t hdivfactor;
};

/* Frame event callback: invoked from the DCMIPP ISR on every frame */

typedef void (*dcmipp_frame_cb_t)(uint32_t pipe);

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

int stm32n6_dcmipp_init(void);
int stm32n6_dcmipp_start_capture(uint32_t dstaddr);
int stm32n6_dcmipp_wait_first_frame(void);
int stm32n6_dcmipp_stop(void);
int stm32n6_dcmipp_set_dstaddr(uint32_t dstaddr);
uint32_t stm32n6_dcmipp_get_frame_count(void);
uint32_t stm32n6_dcmipp_get_csi_sr0(void);
uint32_t stm32n6_dcmipp_get_csi_sr1(void);
void stm32n6_dcmipp_clear_csi_flags(uint32_t sr0_mask,
                        uint32_t sr1_mask);
uint32_t stm32n6_dcmipp_get_error_code(void);
int stm32n6_dcmipp_recover_vc0(void);
uint32_t stm32n6_dcmipp_get_overrun_count(void);
void stm32n6_dcmipp_set_frame_callback(dcmipp_frame_cb_t cb);
int stm32n6_dcmipp_isr(int irq, void *context, void *arg);
int stm32n6_dcmipp_csi_isr(int irq, void *context, void *arg);
void stm32n6_dcmipp_dump_status(void);
void stm32n6_dcmipp_set_rx_mode(int mode);
int stm32n6_dcmipp_set_phy_bitrate(int mbps);
int stm32n6_dcmipp_phy_reinit(void);
int stm32n6_dcmipp_get_rx_mode(void);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_DCMIPP_H */
