/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_wwdg.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_WWDG_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_WWDG_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_wwdg_initialize
 *
 * Description:
 *   Register the WWDG as a NuttX watchdog character device.  The device is
 *   created stopped; user code starts it via the WDIOC_START ioctl.  The
 *   WWDG early-wakeup interrupt backs the optional capture() op.
 *
 * Input Parameters:
 *   devpath - Character device path (e.g. "/dev/watchdog1").
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_wwdg_initialize(const char *devpath);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_WWDG_H */
