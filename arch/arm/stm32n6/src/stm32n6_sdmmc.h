/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_sdmmc.h
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
 * STM32N6 SDMMC driver header.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_SDMMC_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_SDMMC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/mmcsd.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Initialize SDMMC and register MMC/SD device
 * @param bus_num SDMMC instance (1 or 2)
 * @return 0 on success
 */

int stm32n6_sdmmc_initialize(int bus_num);

/**
 * @brief Deinitialize SDMMC
 * @param bus_num SDMMC instance
 * @return 0 on success
 */

int stm32n6_sdmmc_deinitialize(int bus_num);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_SDMMC_H */
