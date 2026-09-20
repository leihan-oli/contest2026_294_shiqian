/****************************************************************************
 * apps/examples/cam_probe/cam_probe_main.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdio.h>

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/* Arch-level camera hardware probe (stm32n6_cam_probe.c). */

int stm32n6_cam_probe(void);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * cam_probe_main
 *
 * Description:
 *   NSH command wrapper around stm32n6_cam_probe(): verifies the DCMIPP
 *   clock/registers and the IMX335 sensor on I2C2.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  printf("cam_probe: running camera hardware probe...\n");

  ret = stm32n6_cam_probe();

  printf("cam_probe: overall %s (ret=%d)\n",
         ret == 0 ? "OK" : "FAIL", ret);

  return ret;
}
