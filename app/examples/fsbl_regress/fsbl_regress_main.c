/****************************************************************************
 * apps/examples/fsbl_regress/fsbl_regress_main.c
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

/* Arch-level FSBL regression probe (stm32n6_fsbl_regress.c). */

int stm32n6_fsbl_regress(void);

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * fsbl_regress_main
 *
 * Description:
 *   NSH command wrapper around stm32n6_fsbl_regress().  Runs the FSBL
 *   regression probe: RIF/LPENR configuration read-back, DMA2D R2M
 *   writes into FLEXMEM and AXISRAM1/2, and a GPDMA1 M2M copy.
 *
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  int ret;

  printf("fsbl_regress: running FSBL regression probe...\n");

  ret = stm32n6_fsbl_regress();

  printf("fsbl_regress: overall %s (ret=%d)\n",
         ret == 0 ? "PASS" : "FAIL", ret);

  return ret;
}
