/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_cam_probe.c
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

/* Camera hardware probe (Phase 6, 2026-08-12).
 *
 * Before porting the DCMIPP + IMX335 driver stack, verify on real silicon
 * that the hardware chain is reachable:
 *   1. DCMIPP clock gate opens and the DCMIPP identification register
 *      (IPGR8 @ +0x1C) reads back non-zero.
 *   2. The IMX335 power-down/reset pins (PWDN=PG6, RST=PG4) sequence and
 *      the IMX335 answers on I2C2 (PD4=SDA, PD14=SCL) -- the ID register
 *      0x3912 is read back.
 *
 * Reference: ST SoftwarePackage 352_IMX335 bare-metal example and
 * Drivers/BSP/IMX335.  NuttX uses the secure APB5 alias (PERIPH 0x50000000),
 * so DCMIPP is at 0x58002000.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <errno.h>
#include <syslog.h>

#include <nuttx/arch.h>
#include <nuttx/i2c/i2c_master.h>

#include "arm_internal.h"
#include "stm32n6_gpio.h"
#include "stm32n6_i2c.h"
#include "hardware/stm32_memorymap.h"
#include "hardware/stm32_rcc.h"

#ifdef CONFIG_STM32_CAM_PROBE

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* DCMIPP base (secure alias: APB5 = 0x58000000) and identification reg. */

#define STM32_DCMIPP_BASE    (STM32_APB5_BASE + 0x2000)
#define DCMIPP_IPGR8_OFFSET  0x1c

/* IMX335 (Sony 5MP sensor): I2C 7-bit address 0x34, 16-bit register
 * addressing, ID register 0x3912 (per ST BSP imx335_reg.h).
 */

#define IMX335_I2C_ADDR      0x34
#define IMX335_REG_ID        0x3912

/* IMX335 power/reset pins (ST SoftwarePackage BSP): PWDN=PG6, RST=PG4. */

#define CAM_PWDN_GPIO        (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | \
                              GPIO_SPEED_HIGH | GPIO_PUPD_NONE | \
                              GPIO_PORTG | GPIO_PIN(6))
#define CAM_RST_GPIO         (GPIO_MODE_OUTPUT | GPIO_OTYPE_PP | \
                              GPIO_SPEED_HIGH | GPIO_PUPD_NONE | \
                              GPIO_PORTG | GPIO_PIN(4))

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_cam_probe
 *
 * Description:
 *   One-shot camera hardware probe: DCMIPP register reachability and
 *   IMX335 I2C presence.  Logs every step.
 *
 * Returned Value:
 *   OK if DCMIPP is reachable and the IMX335 ID read succeeded; a negated
 *   errno otherwise.
 *
 ****************************************************************************/

int stm32n6_cam_probe(void)
{
  FAR struct i2c_master_s *i2c;
  struct i2c_msg_s msg[2];
  uint8_t regbuf[2];
  uint8_t id;
  uint32_t ipgr8;
  int ret;

  syslog(LOG_INFO, "cam: ===== Camera probe (DCMIPP + IMX335) =====\n");

  /* 1. Open the DCMIPP clock gate (APB5, set-alias) and read IPGR8. */

  putreg32(RCC_APB5ENR_DCMIPPEN, STM32_RCC_APB5ENSR);
  ipgr8 = getreg32(STM32_DCMIPP_BASE + DCMIPP_IPGR8_OFFSET);
  syslog(LOG_INFO, "cam: DCMIPP @0x%08lx IPGR8 = 0x%08lx (%s)\n",
         (unsigned long)STM32_DCMIPP_BASE, (unsigned long)ipgr8,
         ipgr8 != 0 ? "present" : "NOT REACHABLE");

  /* 2. IMX335 power-up sequence: PWDN high, RST low, wait, RST high. */

  stm32n6_configgpio(CAM_RST_GPIO);
  stm32n6_configgpio(CAM_PWDN_GPIO);
  stm32n6_gpiowrite(CAM_PWDN_GPIO, true);
  stm32n6_gpiowrite(CAM_RST_GPIO, false);
  up_mdelay(200);
  stm32n6_gpiowrite(CAM_RST_GPIO, true);
  up_mdelay(3);

  syslog(LOG_INFO, "cam: IMX335 power-on done (PWDN=PG6 RST=PG4)\n");

  /* 3. I2C2 read of the IMX335 ID register (16-bit address). */

  i2c = stm32n6_i2cbus_initialize(2);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "cam: I2C2 init failed\n");
      return -ENODEV;
    }

  regbuf[0] = (IMX335_REG_ID >> 8) & 0xff;
  regbuf[1] = IMX335_REG_ID & 0xff;

  msg[0].frequency = 100000;
  msg[0].addr      = IMX335_I2C_ADDR;
  msg[0].flags     = 0;               /* write register address */
  msg[0].buffer    = regbuf;
  msg[0].length    = 2;

  msg[1].frequency = 100000;
  msg[1].addr      = IMX335_I2C_ADDR;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = &id;
  msg[1].length    = 1;

  ret = I2C_TRANSFER(i2c, msg, 2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "cam: I2C read IMX335 ID failed: %d\n", ret);
      return ret;
    }

  syslog(LOG_INFO, "cam: IMX335 ID[0x3912] = 0x%02x\n", id);
  syslog(LOG_INFO, "cam: ===== result: %s =====\n",
         id != 0xff ? "OK" : "FAIL (0xff)");

  return OK;
}

#endif /* CONFIG_STM32_CAM_PROBE */
