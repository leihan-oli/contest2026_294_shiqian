# ##############################################################################
# arch/arm/src/stm32n6/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with this
# work for additional information regarding copyright ownership.  The ASF
# licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
# ##############################################################################

set(SRCS
    stm32n6_start.c
    stm32n6_rcc.c
    stm32n6_lowputc.c
    stm32n6_serial.c
    stm32n6_irq.c
    stm32n6_gpio.c
    stm32n6_timerisr.c
    stm32n6_pwr.c
    stm32n6_idle.c)

if(CONFIG_VIDEO)
  list(APPEND SRCS stm32n6_dcmipp.c)
endif()

if(CONFIG_STM32_IMX335)
  list(APPEND SRCS stm32n6_imx335.c)
endif()

if(CONFIG_STM32_GT9XXX)
  list(APPEND SRCS stm32n6_gt9xxx.c)
  if(CONFIG_STM32N6_GT9XXX_INPUT)
    list(APPEND SRCS stm32n6_gt9xxx_input.c)
  endif()
endif()

if(CONFIG_STM32N6_LCD)
  list(APPEND SRCS stm32n6_lcd.c)
endif()

if(CONFIG_STM32_DCMIPP)
  list(APPEND SRCS stm32n6_dcmipp.c)
endif()

if(CONFIG_STM32N6_VIDEO)
  list(APPEND SRCS stm32n6_video.c)
endif()

if(CONFIG_STM32N6_ATON_AIE)
  list(APPEND SRCS stm32n6_aton_aie.c)

  # The ATON driver hands epoch blocks to the NPU and services its interrupts,
  # so it belongs to the same "no optimisation" set as libs/ai_aton: the
  # board-wide -Os changed the inference results (see the comment in
  # nuttx/libs/ai_aton/CMakeLists.txt).


  # The driver uses the vendored ST ATON runtime API (stai.h) when it is
  # enabled, so pull in the middleware's public include directory.

  if(CONFIG_LIB_AI_ATON)
    target_include_directories(
      arch PRIVATE ${CMAKE_CURRENT_LIST_DIR}/../../../../libs/ai_aton/vendor/Inc
                          ${CMAKE_CURRENT_LIST_DIR}/../../../../libs/ai_aton/nuttx)
  endif()
endif()

# The AXI cache controller (256KB CACHEAXI RAM window used by the NPU) is
# driven by the ATON runtime through libs/ai_aton/npu_cache.c.

if(CONFIG_LIB_AI_ATON)
  list(APPEND SRCS stm32n6_cacheaxi.c)
endif()

if(CONFIG_VIDEO_FB)
  list(APPEND SRCS stm32n6_ltdc.c)
endif()

if(CONFIG_STM32_SDMMC1)
  list(APPEND SRCS stm32n6_sdmmc.c)
endif()

if(CONFIG_STM32_RNG)
  list(APPEND SRCS stm32n6_rng.c)
endif()

if(CONFIG_STM32_RTC)
  list(APPEND SRCS stm32n6_rtc.c)
  if(CONFIG_RTC_DRIVER)
    list(APPEND SRCS stm32n6_rtc_lowerhalf.c)
  endif()
endif()

if(CONFIG_DEV_GPIO)
  list(APPEND SRCS stm32n6_gpio_lower.c)
endif()

if(CONFIG_STM32_XSPI1 OR CONFIG_STM32_XSPI2)
  list(APPEND SRCS stm32n6_xspi.c)
endif()

if(CONFIG_STM32_TIM2)
  list(APPEND SRCS stm32n6_tim.c)
endif()

if(CONFIG_STM32_TIM5)
  list(APPEND SRCS stm32n6_oneshot.c)
endif()

if(CONFIG_STM32_LPTIM1)
  list(APPEND SRCS stm32n6_lptim.c)
endif()

if(CONFIG_STM32_ADC1)
  list(APPEND SRCS stm32n6_adc.c)
endif()

if(CONFIG_STM32_DTS)
  list(APPEND SRCS stm32n6_dts.c)
endif()

if(CONFIG_STM32_HASH)
  list(APPEND SRCS stm32n6_hash.c)
endif()

if(CONFIG_STM32_DMA2D)
  list(APPEND SRCS stm32n6_dma2d.c)
endif()

if(CONFIG_STM32_GPDMA)
  list(APPEND SRCS stm32n6_dma.c)
endif()

if(CONFIG_STM32_SAI1)
  list(APPEND SRCS stm32n6_sai.c)
endif()

if(CONFIG_STM32_FSBL_REGRESS)
  list(APPEND SRCS stm32n6_fsbl_regress.c)
endif()

if(CONFIG_STM32_CAM_PROBE)
  list(APPEND SRCS stm32n6_cam_probe.c)
endif()

if(CONFIG_STM32_I2C1 OR CONFIG_STM32_I2C2 OR CONFIG_STM32_I2C4)
  list(APPEND SRCS stm32n6_i2c.c)
endif()

if(CONFIG_STM32_IWDG)
  list(APPEND SRCS stm32n6_iwdg.c)
endif()

if(CONFIG_STM32_WWDG)
  list(APPEND SRCS stm32n6_wwdg.c)
endif()

target_sources(arch PRIVATE ${SRCS})

# NOTE: this driver stays on the image-wide -Os on purpose.
#
# It was briefly pinned to -O0 to match r62, first with source properties here
# and then with "#pragma GCC optimize (\"O0\")" in the file itself.  The source
# property has no effect at all (CMake only honours it for targets created in
# the same directory, and "arch" is created upstream), and the pragma trapped
# at boot: the compiler emitted "udf #255" for an unreachable path inside
# stm32n6_aton_ns_setup() and the CPU reached it.  GCC's documentation is
# explicit that the optimize attribute/pragma is for debugging only, so the
# -O0 pin lives in libs/ai_aton instead, where it is a plain
# target_compile_options() on the library target.
