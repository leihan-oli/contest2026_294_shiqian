/****************************************************************************
 * arch/arm/src/stm32n6/hardware/stm32_dts.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_DTS_H
#define __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_DTS_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include "hardware/stm32_memorymap.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The STM32N6 DTS is a PVT (process/voltage/temperature) sensor block with
 * a serial data interface (SDIF) to its internal sensors.  Only the subset
 * of registers used by the polling temperature driver is defined here.
 * Sensor 0 (die junction) lives at DTS base + 0xc0.
 */

/* Controller register addresses ********************************************/

#define STM32_DTS_PVT_IER       (STM32_DTS_BASE + 0x0040) /* PVT IRQ enable */
#define STM32_DTS_TSCCLKSYNTHR  (STM32_DTS_BASE + 0x0080) /* Clock synth */
#define STM32_DTS_TSCSDIFDIS    (STM32_DTS_BASE + 0x0084) /* SDIF disable */
#define STM32_DTS_TSCSDIF_SR    (STM32_DTS_BASE + 0x0088) /* SDIF status */
#define STM32_DTS_TSCSDIF_CR    (STM32_DTS_BASE + 0x008c) /* SDIF control */
#define STM32_DTS_TSCSDIF_CFGR  (STM32_DTS_BASE + 0x0094) /* SDIF config */

/* Sensor 0 register addresses (bank at DTS base + 0xc0) ********************/

#define STM32_DTS_S0_BASE       (STM32_DTS_BASE + 0x00c0)
#define STM32_DTS_S0_DONER      (STM32_DTS_S0_BASE + 0x0014) /* Sample done */
#define STM32_DTS_S0_DATAR      (STM32_DTS_S0_BASE + 0x0018) /* Sample data */
#define STM32_DTS_S0_HILORESETR (STM32_DTS_S0_BASE + 0x002c) /* Hi/lo reset */

/* Register bit definitions *************************************************/

/* PVT_IER */

#define DTS_PVT_IER_TS_IRQ_ENABLE  (1 << 1)  /* TS IRQ enable */

/* TSCCLKSYNTHR: TS clock = hsi_div8_ck (8MHz) / ((HI+1)+(LO+1)).  HI=LO=0
 * gives 4MHz, the frequency the temperature formula assumes.  HOLD is the
 * input/output delay field at bit 16; CLK_SYNTH_EN enables the synthesizer.
 */

#define DTS_TSCCLKSYNTHR_CLK_SYNTH_HOLD  (1 << 16) /* HOLD delay (1) */
#define DTS_TSCCLKSYNTHR_CLK_SYNTH_EN    (1 << 24) /* Synthesizer enable */

/* TSCSDIFDIS */

#define DTS_TSCSDIFDIS_TS0_DISABLE  (1 << 0)  /* TS0 SDIF disable */
#define DTS_TSCSDIFDIS_TS1_DISABLE  (1 << 1)  /* TS1 SDIF disable */

/* TSCSDIF_SR */

#define DTS_TSCSDIF_SR_SDIF_BUSY  (1 << 0)  /* SDIF busy flag */

/* TSCSDIF_CR: an SDA (serial data) programming request is issued by writing
 * SDIF_PROG | SDIF_WRN | <SDA register code> | <value>.
 */

#define DTS_TSCSDIF_CR_SDIF_WRN   (1 << 27) /* Write (no read) control */
#define DTS_TSCSDIF_CR_SDIF_PROG  (1 << 31) /* Program request */

/* TSCSDIF_CFGR: inhibit serial programming of the other sensor */

#define DTS_TSCSDIF_CFGR_INHIBIT_0  (1 << 0)  /* Inhibit sensor 0 */
#define DTS_TSCSDIF_CFGR_INHIBIT_1  (2 << 0)  /* Inhibit sensor 1 */

/* DONER (per sensor) */

#define DTS_DONER_SMPL_DONE  (1 << 0)  /* Sample done flag */

/* DATAR (per sensor) */

#define DTS_DATAR_SAMPLE_DATA_MASK  (0xffff << 0) /* Sample data */
#define DTS_DATAR_SAMPLE_TYPE       (1 << 16)     /* Sample type */
#define DTS_DATAR_SAMPLE_FAULT      (1 << 17)     /* Sample fault */

/* HILORESETR (per sensor): reset min/max trackers before a run */

#define DTS_HILORESETR_SMPL_LO_SET  (1 << 0)  /* Set low sample */
#define DTS_HILORESETR_SMPL_HI_CLR  (1 << 1)  /* Clear high sample */

/* SDA (serial data) register codes, OR'd into TSCSDIF_CR *******************/

#define DTS_SDA_CR_REG      0x00000000  /* SDA TS control register */
#define DTS_SDA_CFGR_REG    0x01000000  /* SDA TS configuration register */
#define DTS_SDA_TIMERR_REG  0x05000000  /* SDA TS timer register */

/* SDA values ***************************************************************/

#define DTS_SDA_POWER_UP_DELAY    256         /* Typical power-up delay */
#define DTS_SDA_SENSOR_POWER_DOWN 0x00000001  /* Power down a sensor */
#define DTS_SDA_MODE_SINGLE       0x00000106  /* Single acquisition */
#define DTS_SDA_MODE_CONTINUOUS   0x0000010a  /* Continuous acquisitions */
#define DTS_SDA_RESOLUTION_12BITS 0x00000001  /* 12-bit sensor resolution */

#endif /* __ARCH_ARM_SRC_STM32N6_HARDWARE_STM32_DTS_H */
