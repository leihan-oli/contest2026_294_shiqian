# ##############################################################################
# apps/examples/eye_cam/CMakeLists.txt
#
# SPDX-License-Identifier: Apache-2.0
#
# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to you under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# ##############################################################################

if(CONFIG_EXAMPLES_EYE_CAM)
  if(CONFIG_GRAPHICS_LVGL)
    # The eye-control panel needs the LVGL fonts in flash.  They are kept in
    # their own translation units because each one is a large const table;
    # splitting them keeps the compiler's memory use reasonable and lets the
    # linker drop any font that is not referenced.
    nuttx_add_application(
      NAME ${CONFIG_EXAMPLES_EYE_CAM_PROGNAME}
      SRCS eye_cam_main.c
           eye_ui_font_48.c
           eye_ui_font_32.c
           eye_ui_font_24.c
           eye_ui_font_64.c
      STACKSIZE ${CONFIG_EXAMPLES_EYE_CAM_STACKSIZE}
      PRIORITY ${CONFIG_EXAMPLES_EYE_CAM_PRIORITY}
      DEPENDS lvgl)
  else()
    # Without LVGL there is no panel, but the classifier itself is still
    # useful: this is the path that has been used for the whole model
    # bring-up, so it must keep building.
    nuttx_add_application(
      NAME ${CONFIG_EXAMPLES_EYE_CAM_PROGNAME}
      SRCS eye_cam_main.c
      STACKSIZE ${CONFIG_EXAMPLES_EYE_CAM_STACKSIZE}
      PRIORITY ${CONFIG_EXAMPLES_EYE_CAM_PRIORITY})
  endif()
endif()
