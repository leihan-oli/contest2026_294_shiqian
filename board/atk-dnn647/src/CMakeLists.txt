# ##############################################################################
# vendor/openvela/boards/atk-dnn647/src/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more contributor
# license agreements.  See the NOTICE file distributed with this work for
# additional information regarding copyright ownership.  The ASF licenses this
# file to you under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License.  You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations under
# the License.
#
# ##############################################################################

set(SRCS board_bringup.c)
list(APPEND SRCS stm32n6_buzzer.c)
list(APPEND SRCS stm32n6_buzzer.c)

if(CONFIG_USERLED_LOWER)
  list(APPEND SRCS stm32n6_userleds.c)
endif()

if(CONFIG_ARCH_BUTTONS)
  list(APPEND SRCS stm32n6_userbuttons.c)
endif()

if(CONFIG_EDGESIGHT_CLOCK_800MHZ)
  list(APPEND SRCS stm32n6_clockconfig.c)
endif()

if(CONFIG_BOARDCTL_RESET_CAUSE)
  list(APPEND SRCS stm32n6_reset.c)
endif()

# Audio output.  Compiled only when the SAI block driver is enabled; the file
# itself is written so that it produces nothing without it.
if(CONFIG_STM32_SAI1)
  list(APPEND SRCS stm32n6_audio.c)
endif()

target_sources(board PRIVATE ${SRCS})

if(CONFIG_ARCH_CHIP_STM32N6)
  set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash.ld")
else()
  set_property(GLOBAL PROPERTY LD_SCRIPT "${NUTTX_BOARD_DIR}/scripts/flash-qemu.ld")
endif()
