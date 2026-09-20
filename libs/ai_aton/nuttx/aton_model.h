/****************************************************************************
 * libs/ai_aton/nuttx/aton_model.h
 *
 * Thin NuttX wrapper around one ATON-compiled network.
 *
 * The driver (arch/arm/src/stm32n6/stm32n6_aton_aie.c) must not know about
 * the ST generated symbols (LL_ATON_Set_User_Input_Buffer_Default(),
 * NN_Instance_Default, ...) - those are produced by ST Edge AI for one
 * specific model and live in libs/ai_aton/models/<model>/.  This module is
 * the only place that binds a model to the runtime and exposes the three
 * things a caller needs: the input tensor size, the output tensor size and
 * a "run once" entry point.
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

#ifndef __LIBS_AI_ATON_NUTTX_ATON_MODEL_H
#define __LIBS_AI_ATON_NUTTX_ATON_MODEL_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: aton_model_init
 *
 * Description:
 *   Query the compiled-in model for its tensor sizes and report what was
 *   linked.  Called once from the ATON driver's session init.
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

/* Model slots.
 *
 * ATON_MODEL_PRIMARY is whichever model CONFIG_AI_ATON_MODEL_* selected
 * (eye, palm995 or pose994).  The eye configuration also links the YuNet
 * face detector that localises the eye for the classifier, and that one is
 * ATON_MODEL_FACE.
 *
 * Every accessor below takes a slot id.  Tensors, buffers and run counters
 * are per model; the runtime state (IRQ count, stage, verbose flag) stays
 * global because there is one NPU and one epoch controller, on which the two
 * models run one after the other.
 */

enum aton_model_id
{
  ATON_MODEL_PRIMARY = 0,
#if defined(CONFIG_AI_ATON_MODEL_EYE)
  ATON_MODEL_FACE,
#endif
  ATON_MODEL_NMODELS
};

int aton_model_init(int id);

/****************************************************************************
 * Name: aton_model_publish
 *
 * Description:
 *   Publish the model's epoch program, i.e. hand the blob in .nsblob to the
 *   epoch controller's loader.  The input buffer has to be registered
 *   (aton_model_set_input_buffer) *before* this call: the program carries the
 *   input address as a relocation entry the loader resolves once, here, so a
 *   buffer registered afterwards never reaches the engines.
 *
 *   Each model has its own loader - the generated symbol carries the model
 *   suffix - and the generated NN_Interface_<suffix> object already holds a
 *   pointer to it, so this dispatches through the descriptor table instead of
 *   naming one model's loader from the driver.
 *
 * Returned Value:
 *   OK when the loader accepted the program; -EIO when it refused it, which
 *   means the linked blob is not runnable.
 *
 ****************************************************************************/

int aton_model_publish(int id);

/****************************************************************************
 * Name: aton_model_name
 *
 * Description:
 *   Name of the compiled-in model (for log/identification purposes).
 *
 ****************************************************************************/

FAR const char *aton_model_name(int id);

/****************************************************************************
 * Name: aton_model_input_len / aton_model_output_len
 *
 * Description:
 *   Size in bytes of the single user input/output tensor of the model.
 *
 ****************************************************************************/

uint32_t aton_model_input_len(int id);
uint32_t aton_model_output_len(int id);

/* Address the model writes its results to when it owns the output (the
 * tensors live inside the NPU pool): the application has to read from *this*
 * address.  0 means the model has a user allocated output and the caller's
 * buffer is used, exactly as reported by aton_model_run()'s output argument.
 */

uintptr_t aton_model_output_addr(int id);
uintptr_t aton_model_input_addr(int id);
uintptr_t aton_model_input_registered(int id);
int aton_model_set_input_buffer(int id, FAR void *buffer, uint32_t size);
uintptr_t aton_model_input_pointer(int id);

/* Publish the secondary epoch blobs in non-secure RAM (r32, see
 * network_ecblobs.h); returns a checksum over the copies.
 */

uint32_t aton_model_ecblobs_to_ns(void);

#if defined(CONFIG_AI_ATON_MODEL_EYE)
/* Same hook, generated into the face model's network.c under its own name
 * because the two files are linked into the same image.
 */

uint32_t aton_model_face_ecblobs_to_ns(void);
#endif

/* Publish the secondary epoch blobs of the model in slot id in non-secure RAM
 * (r32, see network_ecblobs.h); returns a checksum over the copies.
 *
 * The hook itself is generated into each model's network.c and keeps its own
 * name (aton_model_ecblobs_to_ns for the primary, aton_model_face_ecblobs_to_ns
 * for the face model); this is the dispatcher over the two.
 */

uint32_t aton_model_blob_sync(int id);

/****************************************************************************
 * Name: aton_model_run
 *
 * Description:
 *   Run one inference: hand the input and output buffers to the ATON
 *   runtime and run the epoch loop to completion.  Cache maintenance is
 *   applied around the transfer because both buffers are normal (cacheable)
 *   CPU memory while the NPU reads/writes them directly.
 *
 * Input Parameters:
 *   input  - Buffer holding aton_model_input_len() bytes
 *   output - Buffer receiving aton_model_output_len() bytes
 *   cycles - Optional out parameter receiving the epoch loop time in
 *            microseconds (may be NULL)
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

int aton_model_run(int id, FAR const void *input, FAR void *output,
                   FAR uint32_t *usec);

/****************************************************************************
 * Name: aton_model_runs
 *
 * Description:
 *   Number of inferences completed since boot.
 *
 ****************************************************************************/

uint32_t aton_model_runs(int id);

/****************************************************************************
 * Name: aton_model_irq_count
 *
 * Description:
 *   Number of epoch-complete events signalled by the NPU interrupt handler
 *   since boot (see aton_osal_nuttx_event_count()).
 *
 ****************************************************************************/

uint32_t aton_model_irq_count(void);

/****************************************************************************
 * Name: aton_model_dump_log
 *
 * Description:
 *   Print the log lines the ATON runtime recorded (its error paths log from
 *   interrupt context into a lock-free ring).  Task context only.
 *
 ****************************************************************************/

void aton_model_dump_log(void);

/****************************************************************************
 * Name: aton_model_report / aton_model_storm_masked
 *
 * Description:
 *   Runtime status for watchdogs: a one-line summary of the last runtime
 *   phase plus the interrupt/event counters, and whether the NPU interrupt
 *   line had to be masked because it was re-asserting without progress.
 *
 ****************************************************************************/

void aton_model_report(FAR const char *tag);
void aton_model_set_verbose(bool verbose);
bool aton_model_storm_masked(void);
int aton_model_stage(void);

#ifdef __cplusplus
}
#endif

#endif /* __LIBS_AI_ATON_NUTTX_ATON_MODEL_H */
