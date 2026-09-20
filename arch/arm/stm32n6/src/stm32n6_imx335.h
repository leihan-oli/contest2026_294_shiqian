/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_imx335.h
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
 * STM32N6 IMX335 (Sony 5MP MIPI CSI-2) sensor driver.
 * Ported from the ST BSP (Drivers/BSP/IMX335/STM32_IMX335) for NuttX.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_IMX335_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_IMX335_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* IMX335 I2C address (7-bit) and ID.
 *
 * The ID register 0x3912 reads back 0x34 on the ALIENTEK STM32N647 board
 * (verified on silicon by cam_probe and cam start; the ALIENTEK example
 * header claims 0x00 which does not match the real part).
 */

#define IMX335_I2C_ADDR          0x1a
#define IMX335_ID                0x00
#define IMX335_REG_ID            0x3912

/* IMX335 register address of the mode select (streaming on/off) */

#define IMX335_REG_MODE_SELECT   0x3000
#define IMX335_MODE_STANDBY      0x01
#define IMX335_MODE_STREAMING    0x00

/* Resolution / mode registers used by this port */

#define IMX335_REG_HREVERSE      0x3001
#define IMX335_REG_VREVERSE      0x3002

/* Input clock selections (37 MHz for the ALIENTEK board) */

#define IMX335_INCK_74MHZ        74
#define IMX335_INCK_37MHZ        37
#define IMX335_INCK_27MHZ        27
#define IMX335_INCK_24MHZ        24
#define IMX335_INCK_18MHZ        18
#define IMX335_INCK_6MHZ         6

/* Resolution identifiers (only 2592x1944 is supported) */

#define IMX335_R2592_1944        1u

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_imx335_power_on
 *
 * Description:
 *   IMX335 power-up sequence: PWDN=1, RST=0, wait, RST=1.  Must be called
 *   before stm32n6_dcmipp_init().
 *
 ****************************************************************************/

int stm32n6_imx335_power_on(void);

/****************************************************************************
 * Name: stm32n6_imx335_configure
 *
 * Description:
 *   Open I2C2, verify the IMX335 ID and program the 2592x1944 RAW10
 *   register tables (power-on already done).  Must be called before
 *   stm32n6_dcmipp_init().
 *
 ****************************************************************************/

int stm32n6_imx335_configure(void);

/****************************************************************************
 * Name: stm32n6_imx335_start_stream
 *
 * Description:
 *   Start the sensor MIPI CSI-2 output (24 MHz crystal, PLL per the
 *   resolution table).  Must be called after stm32n6_dcmipp_init().
 *
 ****************************************************************************/

int stm32n6_imx335_start_stream(void);

/****************************************************************************
 * Name: stm32n6_imx335_init
 *
 * Description:
 *   Open I2C2, check the IMX335 ID, program the 2592x1944 RAW10 register
 *   tables and start streaming.  Must be called after
 *   stm32n6_dcmipp_init().
 *
 ****************************************************************************/

int stm32n6_imx335_init(void);

/****************************************************************************
 * Name: stm32n6_imx335_read_reg
 *
 * Description:
 *   Read one byte from an IMX335 register (16-bit register address).
 *   Requires the sensor to have been initialized (I2C bus open).
 *
 ****************************************************************************/

int stm32n6_imx335_read_reg(uint16_t reg, FAR uint8_t *val);

int stm32n6_imx335_write_reg(uint16_t reg, uint8_t val);

int stm32n6_imx335_read_reg_8bit(uint8_t reg, FAR uint8_t *val);
int stm32n6_imx335_write_reg_8bit(uint8_t reg, uint8_t val);

int stm32n6_imx335_i2c_scan(void);

/* Runtime gain / exposure / frame-length overrides.  The setters only
 * record the value; it is written to the sensor by
 * stm32n6_imx335_configure() (through stm32n6_imx335_apply_tuning()) so
 * that it survives the reset performed by each capture session.  Pass 0 to
 * drop the override and fall back to the register table value.
 */

void stm32n6_imx335_set_gain(uint32_t gain);
void stm32n6_imx335_set_exposure(uint32_t shr0);
void stm32n6_imx335_set_vmax(uint32_t vmax);

/****************************************************************************
 * Name: stm32n6_imx335_apply_tuning
 *
 * Description:
 *   Write the recorded overrides to the sensor.  Called at the end of
 *   stm32n6_imx335_configure(), and usable directly by tuning tools.
 *
 ****************************************************************************/

int stm32n6_imx335_apply_tuning(void);

/****************************************************************************
 * Name: stm32n6_imx335_set_hs_mode / stm32n6_imx335_get_hs_mode
 *
 * Description:
 *   Select which MIPI D-PHY HS transmit-timing registers the next
 *   stm32n6_imx335_start_stream() writes:
 *     0 = none (sensor defaults), 1 = clock lane (0x3a18-0x3a1e),
 *     2 = data lane (0x3a20-0x3a28), 3 = both (default).
 *
 ****************************************************************************/

void stm32n6_imx335_set_hs_mode(int mode);
int stm32n6_imx335_get_hs_mode(void);

int stm32n6_imx335_read_reg_at(uint16_t i2c_addr, uint16_t reg,
                             FAR uint8_t *val);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_IMX335_H */
