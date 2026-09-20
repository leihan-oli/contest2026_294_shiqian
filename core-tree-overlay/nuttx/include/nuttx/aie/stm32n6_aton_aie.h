/****************************************************************************
 * include/nuttx/aie/stm32n6_aton_aie.h
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
 ****************************************************************************/

#ifndef __INCLUDE_NUTTX_AIE_STM32N6_ATON_AIE_H
#define __INCLUDE_NUTTX_AIE_STM32N6_ATON_AIE_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <stdint.h>
#include <stddef.h>
#include <nuttx/aie/ai_engine.h>

/****************************************************************************
 * Public Types
 ****************************************************************************/

/* Model slots.  The ATON runtime binds to its model at build time, so a
 * build either has one model (slot 0) or a pair (slot 0 plus the companion
 * slot 1 - in the eye configuration, the classifier plus the face detector
 * that localises it).  The values mirror enum aton_model_id in
 * nuttx/libs/ai_aton/nuttx/aton_model.h; keep the two in sync.
 *
 * A build without the second model answers -ENODEV for slot 1.
 */

#define STM32N6_ATON_MODEL_PRIMARY   0
#define STM32N6_ATON_MODEL_FACE      1
#define STM32N6_ATON_MODEL_NMODELS   2

/* Structure to pass STM32N6 ATON inference parameters through the AIE
 * ioctl.  This mirrors the Ethos-U ethosu_aie_invoke_params pattern: the
 * AIE upper-half passes a single opaque address, and the lower-half
 * interprets it as a driver-specific parameter block.
 */

struct stm32n6_aton_invoke_params
{
  FAR const void *model;        /* ATON model (compiled-in C array) */
  FAR const void *input;        /* Input tensor base address */
  FAR void *output;             /* Output tensor base address */
  size_t input_size;            /* Input tensor size in bytes */
  size_t output_size;           /* Output tensor size in bytes */

  /* Which slot to run.  The two models of an eye build share one input
   * buffer (the driver reports its address through GET_STATUS), so the only
   * thing that selects between them is this field.
   */

  uint32_t model_id;            /* STM32N6_ATON_MODEL_*, 0 when unset */

  /* Filled in by the lower half (AIE_CMD_FEED_INPUT) */

  int      result;              /* 0 on success, else a negated errno */
  uint32_t usec;                /* Epoch loop duration in microseconds */
  uint32_t runs;                /* Inferences of *this* slot since boot */
  uint32_t events;              /* NPU epoch-complete events signalled */
};

/* STM32N6 ATON driver status (AIE_CMD_.. control, arg = pointer to this).
 *
 * model_id is an input field: the caller selects the slot it wants described
 * and the lower half fills the rest.  Slot 0 is assumed when it is left at
 * zero, which is what a caller written against the one-model driver expects.
 */

struct stm32n6_aton_status_s
{
  uint32_t model_id;            /* IN: STM32N6_ATON_MODEL_* */
  FAR const char *name;         /* OUT: the slot's model name */
  uint32_t input_size;          /* Model input tensor size in bytes */
  uint32_t output_size;         /* Model output tensor size in bytes */
  uint32_t input_addr;         /* Address the epoch program reads the input
                                * from.  It was resolved once, when the program
                                * was published, so this is where the caller
                                * has to put the frame; filling any other
                                * buffer feeds the engines nothing.  0 when
                                * the model has no input tensor */
  uint32_t output_addr;         /* Address of the output inside the NPU pool,
                                 * 0 when the caller supplies the buffer */
  uint32_t runs;                /* Inferences of this slot since boot */
  uint32_t last_usec;           /* Duration of its last inference */
  uint32_t events;              /* NPU epoch-complete events signalled */
  int      last_result;         /* Result of its last inference */
};

/* Driver-specific control commands forwarded by the AIE upper half to the
 * lower-half control() callback (values must not collide with AIE_CMD_*).
 */

#define STM32N6_ATON_CMD_GET_STATUS  0x100
#define STM32N6_ATON_CMD_RUN         0x101

/* Write a string to the console with polled, unbuffered output.  The string
 * is formatted by the caller and passed as a pointer in the ioctl argument.
 *
 * This exists so diagnostics can be printed without going through the console
 * driver (and its TX semaphore): if that path stalls, everything that prints
 * through it blocks, while this keeps working.
 */

#define STM32N6_ATON_CMD_PUTS        0x102   /* arg = string pointer */

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_aton_aie_initialize
 *
 * Description:
 *   Initialize the STM32N6 Neural-ART NPU driver and register it with the
 *   AI Engine upper half.  This function is called during board
 *   initialization to set up the NPU hardware support.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   OK (0) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int stm32n6_aton_aie_initialize(void);

#endif /* __INCLUDE_NUTTX_AIE_STM32N6_ATON_AIE_H */
