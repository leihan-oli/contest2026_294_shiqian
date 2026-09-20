# ##############################################################################
# apps/audioutils/speexdsp/CMakeLists.txt
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
if(CONFIG_AUDIOUTILS_SPEEXDSP)

  if(NOT EXISTS ${CMAKE_CURRENT_LIST_DIR}/speexdsp)
    if(NOT SPEEXDSP_URL)
      set(SPEEXDSP_URL "file:///home/leihann/openvela/local_src/speexdsp")
    endif()

    if(NOT SPEEXDSP_VERSION)
      set(SPEEXDSP_VERSION "SpeexDSP-1.2.1")
    endif()

    FetchContent_Declare(
      speexdsp
      URL "${SPEEXDSP_URL}/${SPEEXDSP_VERSION}.zip"
      URL_HASH SHA256=be16e16bcbc26875916a8eedd3b5b46cb93bef4714c49895a28f8c86b81437cf
          SOURCE_DIR
          ${CMAKE_CURRENT_LIST_DIR}/speexdsp
          BINARY_DIR
          ${CMAKE_BINARY_DIR}/apps/audioutils/speexdsp/speexdsp
          CONFIGURE_COMMAND
          ""
          BUILD_COMMAND
          ""
          INSTALL_COMMAND
          ""
          TEST_COMMAND
          ""
      DOWNLOAD_NO_PROGRESS true
      TIMEOUT 30)

    FetchContent_GetProperties(speexdsp)

    if(NOT speexdsp_POPULATED)
      FetchContent_Populate(speexdsp)
    endif()
  endif()

  set(CSRCS speexdsp/libspeexdsp/resample.c)

  nuttx_add_library(libspeex STATIC)
  target_sources(
    libspeex
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/speexdsp/libspeexdsp/resample.c)
  target_include_directories(
    libspeex PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/speexdsp/include/speex)

  target_compile_definitions(libspeex PRIVATE OUTSIDE_SPEEX RANDOM_PREFIX=nuttx)

  if(CONFIG_FIXED_POINT)
    target_compile_definitions(libspeex PRIVATE FIXED_POINT)
  else()
    target_compile_definitions(libspeex PRIVATE FLOATING_POINT)
  endif()

  set_property(
    TARGET nuttx
    APPEND
    PROPERTY NUTTX_INCLUDE_DIRECTORIES
             ${CMAKE_CURRENT_SOURCE_DIR}/speexdsp/include/speex)

endif()
