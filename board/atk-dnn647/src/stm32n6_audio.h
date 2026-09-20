/****************************************************************************
 * vendor/openvela/boards/atk-dnn647/src/stm32n6_audio.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 * ATK-DNN647 audio output: ES8388 codec on SAI1.
 *
 ****************************************************************************/

#ifndef __VENDOR_OPENVELA_BOARDS_ATK_DNN647_SRC_STM32N6_AUDIO_H
#define __VENDOR_OPENVELA_BOARDS_ATK_DNN647_SRC_STM32N6_AUDIO_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_audio_init
 *
 * Description:
 *   Bring up the codec and the transmit path.  Idempotent: the codec is
 *   configured once and left running, because its own startup takes a
 *   hundred milliseconds and a prompt should not wait for that.
 *
 * Returned Value:
 *   Zero on success, a negated errno on failure.
 *
 ****************************************************************************/

int stm32n6_audio_init(void);

/****************************************************************************
 * Name: stm32n6_audio_play_wav
 *
 * Description:
 *   Play a 16-bit PCM WAV file from the filesystem.
 *
 *   Blocks until the file has been sent.  Playback is a single writer: a
 *   prompt is short and the speaker has one channel, so overlapping calls
 *   would only garble each other.
 *
 * Returned Value:
 *   Zero on success, a negated errno on failure.
 *
 ****************************************************************************/

int stm32n6_audio_play_wav(FAR const char *path);

/****************************************************************************
 * Name: stm32n6_audio_set_volume
 *
 * Description:
 *   Set the speaker volume, 0 to 33.  The codec's own range, not a
 *   percentage.
 *
 ****************************************************************************/

void stm32n6_audio_set_volume(uint8_t volume);

#endif /* __VENDOR_OPENVELA_BOARDS_ATK_DNN647_SRC_STM32N6_AUDIO_H */
