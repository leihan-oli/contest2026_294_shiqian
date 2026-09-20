# ##############################################################################
# libs/ai_aton/CMakeLists.txt
#
# ST Neural-ART (ATON) runtime: our NuttX OSAL glue, the NuttX device glue
# (CPU/NPU cache maintenance) and the vendored ST middleware drop under
# vendor/ (headers, the LL/RT sources the middleware ships, and the prebuilt
# NetworkRuntime archive).
#
# The build recipe (source list, platform and OSAL selection) mirrors the ST
# application note examples, e.g. Projects/99_Applications/994_AI_Multi_Pose_
# Estimation, which builds:
#
#   Npu/Devices/STM32N6XX/{mcu_cache,npu_cache}.c
#   Npu/ll_aton/{ecloader,ll_aton,ll_aton_cipher,ll_aton_dbgtrc,
#                ll_aton_debug,ll_aton_lib,ll_aton_lib_sw_operators,
#                ll_aton_n64,ll_aton_profiler,ll_aton_rt_main,
#                ll_aton_runtime,ll_aton_util,ll_sw_float,ll_sw_integer}.c
#   Lib/GCC/ARMCortexM55/NetworkRuntime1000_CM55_GCC.a
#
# with -DLL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6, -DLL_ATON_SW_FALLBACK and an
# OSAL selection.  We use LL_ATON_OSAL_USER_IMPL (ll_aton_osal_nuttx.c) instead
# of one of the vendor RTOS ports, and our own mcu_cache/npu_cache glue so that
# no ST Cube HAL/CMSIS device tree is required.
#
# The prebuilt archive is linked through nuttx_add_extra_library() so the
# final ELF picks it up by full path.
#
# SPDX-License-Identifier: Apache-2.0
#
# ##############################################################################

if(CONFIG_LIB_AI_ATON)

  set(AI_ATON_DIR ${CMAKE_CURRENT_LIST_DIR})

  # NuttX-side sources: the OSAL implementation (the middleware is told to use
  # it through LL_ATON_OSAL=LL_ATON_OSAL_USER_IMPL) and the cache maintenance
  # glue for the CPU D-Cache and the NPU AXI cache.

  # The selected model (generated network.c + epoch-controller blobs) is bound
  # to the runtime by nuttx/aton_model.c, which is the only place that knows
  # the ST generated symbol names.

  set(AI_ATON_MODEL_SRCS)

  if(CONFIG_AI_ATON_MODEL_POSE994)
    list(APPEND AI_ATON_MODEL_SRCS nuttx/aton_model.c
                                   models/pose994/network.c)
  elseif(CONFIG_AI_ATON_MODEL_PALM995)
    list(APPEND AI_ATON_MODEL_SRCS nuttx/aton_model.c
                                   models/palm995/network.c)
  elseif(CONFIG_AI_ATON_MODEL_EYE)
    # Dual model: the eye classifier plus the YuNet face detector that
    # localises the eye before the crop.  They use different symbol
    # families (_Default / _Face) so both can be linked into one image;
    # the NPU runs them one after the other on the same epoch
    # controller.
    list(APPEND AI_ATON_MODEL_SRCS nuttx/aton_model.c
                                   models/eye/network.c
                                   models/face/network.c)
  endif()

  nuttx_add_library(
    ai_aton
    ll_aton_osal_nuttx.c
    nuttx/mcu_cache.c
    nuttx/npu_cache.c
    vendor/ll_aton/ecloader.c
    vendor/ll_aton/ll_aton.c
    vendor/ll_aton/ll_aton_cipher.c
    vendor/ll_aton/ll_aton_dbgtrc.c
    vendor/ll_aton/ll_aton_debug.c
    vendor/ll_aton/ll_aton_lib.c
    vendor/ll_aton/ll_aton_lib_sw_operators.c
    vendor/ll_aton/ll_aton_n64.c
    vendor/ll_aton/ll_aton_profiler.c
    vendor/ll_aton/ll_aton_rt_main.c
    vendor/ll_aton/ll_aton_runtime.c
    vendor/ll_aton/ll_aton_util.c
    vendor/ll_aton/ll_sw_float.c
    vendor/ll_aton/ll_sw_integer.c
    ${AI_ATON_MODEL_SRCS})

  # compat/ must come first: the vendor platform header does a quoted
  # #include "stm32n6xx.h" which has to resolve to our shim.
  #
  # nuttx/ carries the NuttX versions of mcu_cache.h / npu_cache.h; the chip
  # directory provides the CACHEAXI driver prototype.

  target_include_directories(
    ai_aton
    PUBLIC ${AI_ATON_DIR}/compat ${AI_ATON_DIR}/nuttx ${AI_ATON_DIR}
           ${AI_ATON_DIR}/vendor/Inc ${AI_ATON_DIR}/vendor/ll_aton
           ${AI_ATON_DIR}/vendor/Devices/STM32N6XX
           ${CMAKE_CURRENT_LIST_DIR}/../../arch/arm/src/stm32n6)

  # NDEBUG is the ST "release" configuration: it removes the runtime's
  # assert()s, which would otherwise panic the target when an epoch error is
  # reported from interrupt context (the error messages are preserved through
  # the OSAL log ring - see vendor/ll_aton/ll_aton_util.h).
  target_compile_definitions(
    ai_aton PRIVATE LL_ATON_PLATFORM=LL_ATON_PLAT_STM32N6
                   LL_ATON_OSAL=LL_ATON_OSAL_USER_IMPL LL_ATON_SW_FALLBACK
                   NDEBUG)

  # Vendored prebuilt runtime: stai_runtime_* + ai_* layer + CMSIS-NN.

  nuttx_add_extra_library(${AI_ATON_DIR}/vendor/lib/NetworkRuntime1000_CM55_GCC.a)


  # Pin the ATON runtime, the OSAL/cache glue and the generated network to -O0
  # while the rest of the board keeps the -Os build it needs to fit the FSBL's
  # 1 MiB LRUN window.
  #
  # After the board-wide -Os switch the ATON results changed: the fill-input
  # regression fingerprints moved from 0x2f0e5723/0xa8345069/0x0955ccd9
  # (r59..r62) to 0x52dbec6b/c75e222d/0x658882e3, and a palm that the PC
  # reference model detects on the very same input tensor (+0.86 logit) came
  # back as "no palm" (-2.59).  Rebuilding just this path without optimisation
  # costs a few tens of KB and removes the variable while the difference is
  # being understood.
  target_compile_options(ai_aton PRIVATE -O0)

endif()
