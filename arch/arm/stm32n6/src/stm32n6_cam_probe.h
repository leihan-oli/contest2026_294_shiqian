/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_cam_probe.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_CAM_PROBE_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_CAM_PROBE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifdef CONFIG_STM32_CAM_PROBE

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_cam_probe
 *
 * Description:
 *   One-shot camera hardware probe: DCMIPP register reachability and
 *   IMX335 I2C presence (ID register 0x3912 on I2C2).
 *
 * Returned Value:
 *   OK on success; a negated errno otherwise.
 *
 ****************************************************************************/

int stm32n6_cam_probe(void);

#endif /* CONFIG_STM32_CAM_PROBE */
#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_CAM_PROBE_H */
