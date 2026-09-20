/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_video.h
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

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_VIDEO_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_VIDEO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_video_initialize
 *
 * Description:
 *   Register the STM32N6 V4L2 camera capture device (/dev/video0) that
 *   wraps the DCMIPP + IMX335 drivers.
 *
 ****************************************************************************/

int stm32n6_video_initialize(void);

/****************************************************************************
 * Name: stm32n6_video_capture_dest / stm32n6_video_set_capture_dest
 *
 * Description:
 *   Read or override where captured frames land.  Zero (the default) means
 *   the board's panel framebuffer; any other value must be a 16-byte aligned
 *   address big enough for one frame.
 *
 ****************************************************************************/

uint32_t stm32n6_video_capture_dest(void);
void stm32n6_video_set_capture_dest(uint32_t addr);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_VIDEO_H */
