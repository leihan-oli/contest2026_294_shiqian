/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_otg.h
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
 * STM32N6 USB OTG HS driver header.
 * Supports device mode for UVC (webcam) and mass storage.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_OTG_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_OTG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* USB OTG mode */

enum usb_otg_mode_e
{
  USB_OTG_MODE_DEVICE = 0,
  USB_OTG_MODE_HOST   = 1,
};

/* USB OTG speed */

enum usb_otg_speed_e
{
  USB_OTG_SPEED_FULL = 0,  /* 12 Mbps */
  USB_OTG_SPEED_HIGH = 1,  /* 480 Mbps */
};

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Initialize USB OTG in device mode
 * @param speed Desired speed (FULL or HIGH)
 * @return 0 on success
 */

int stm32n6_otg_device_init(enum usb_otg_speed_e speed);

/**
 * @brief Initialize USB OTG in host mode
 * @return 0 on success
 */

int stm32n6_otg_host_init(void);

/**
 * @brief Deinitialize USB OTG
 * @return 0 on success
 */

int stm32n6_otg_deinit(void);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_OTG_H */
