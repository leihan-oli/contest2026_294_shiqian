/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_ltdc.h
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
 * STM32N6 LTDC display controller driver header.
 *
 ****************************************************************************/

#ifndef __ARCH_ARM_SRC_STM32N6_STM32N6_LTDC_H
#define __ARCH_ARM_SRC_STM32N6_STM32N6_LTDC_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdint.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * @brief Initialize LTDC display controller
 * @param width Display width
 * @param height Display height
 * @param bg_buf Background framebuffer (camera output)
 * @param fg_buf1 Foreground framebuffer 1 (overlay)
 * @param fg_buf2 Foreground framebuffer 2 (double-buffer)
 * @return OK on success
 */

int stm32n6_ltdc_init(uint32_t width, uint32_t height,
                        void *bg_buf, void *fg_buf1,
                        void *fg_buf2);

/**
 * @brief Update background layer buffer address
 * @param buffer New framebuffer address
 * @return OK on success
 */

int stm32n6_ltdc_set_bg_buffer(void *buffer);

/**
 * @brief Swap foreground double buffer
 * @return Pointer to buffer now safe to write
 */

void *stm32n6_ltdc_swap_fg_buffer(void);

/**
 * @brief Fill a rectangle on the foreground layer
 * @param x X coordinate
 * @param y Y coordinate
 * @param w Width
 * @param h Height
 * @param color RGB565 color value
 * @return OK on success
 */

int stm32n6_ltdc_fill_fg_rect(uint32_t x, uint32_t y,
                                uint32_t w, uint32_t h,
                                uint16_t color);

/**
 * @brief Clear foreground layer to transparent
 */

void stm32n6_ltdc_clear_fg(void);

/**
 * @brief Get pointer to safe-to-write foreground buffer
 * @return Buffer pointer
 */

void *stm32n6_ltdc_get_fg_buffer(void);

#endif /* __ARCH_ARM_SRC_STM32N6_STM32N6_LTDC_H */
