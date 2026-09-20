# ##############################################################################
# apps/examples/pose_cam/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.  The
# ASF licenses this file to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance with the
# License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
# License for the specific language governing permissions and limitations
# under the License.
#
# ##############################################################################

if(CONFIG_EXAMPLES_PALM_CAM)
  nuttx_add_application(
    NAME ${CONFIG_EXAMPLES_PALM_CAM_PROGNAME}
    SRCS palm_cam_main.c
    STACKSIZE ${CONFIG_EXAMPLES_PALM_CAM_STACKSIZE}
    PRIORITY ${CONFIG_EXAMPLES_PALM_CAM_PRIORITY})

  # The probe belongs to the "no optimisation" set of the ATON path: it
  # prepares the tensor, drives the engine and decodes the pool, and the
  # board-wide -Os changed the inference results (see the comment in
  # nuttx/libs/ai_aton/CMakeLists.txt).  The preprocessing loop is the hottest
  # CPU code of the whole application, so this also keeps its timing
  # comparable with the r59..r62 measurements.
  #
  # A source-file property is used because the application target is created in
  # the apps directory scope, where target_compile_options() cannot reach it.

  set_source_files_properties(palm_cam_main.c PROPERTIES COMPILE_OPTIONS "-O0")
endif()
