/****************************************************************************
 * libs/ai_aton/nuttx/aton_model.c
 *
 * Binds the ATON-compiled network to the ATON runtime.  See aton_model.h.
 *
 * The call sequence is the one ST's application examples use
 * (Projects/99_Applications/994_AI_Multi_Pose_Estimation/Appli/Core/Src/
 * app.c):
 *
 *   LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(Default);
 *   nn_in_len  = LL_Buffer_len(LL_ATON_Input_Buffers_Info_Default());
 *   nn_out_len = LL_Buffer_len(LL_ATON_Output_Buffers_Info_Default());
 *   LL_ATON_Set_User_Input_Buffer_Default(0, in, nn_in_len);
 *   LL_ATON_Set_User_Output_Buffer_Default(0, out, nn_out_len);
 *   LL_ATON_RT_Main(&NN_Instance_Default);
 *
 * LL_ATON_RT_Main() is self-contained: it initializes the runtime and the
 * ATON IPs, installs the epoch-complete interrupt handler through the OSAL
 * hook (LL_ATON_OSAL_INSTALL_IRQ), runs the epoch blocks (waiting in
 * LL_ATON_OSAL_WFE() for the NPU interrupt), and de-initializes again.
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <debug.h>
#include <stdbool.h>
#include <stdint.h>

#include <nuttx/cache.h>
#include <nuttx/clock.h>

#include "ll_aton.h"
#include "ll_aton_lib.h"
#include "ll_aton_NN_interface.h"

#include "ll_aton_runtime.h"

#include "ll_aton_osal_user_impl.h"

#include "aton_model.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Model name as produced by ST Edge AI (--network-name).  Every generated
 * symbol carries that suffix (LL_ATON_Set_User_Input_Buffer_<name>,
 * NN_Instance_<name>, ...).  The suffix has to be spelled out literally: the
 * declaration macros paste it with ##, which does not expand an alias.
 */

#if defined(CONFIG_AI_ATON_MODEL_POSE994)
#  define ATON_MODEL_STR  "pose994 (yolov8n-pose 256x256, ST 994_AI_Multi_Pose_Estimation)"
#  define ATON_MODEL_DECLARE() \
     LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(Default)
#elif defined(CONFIG_AI_ATON_MODEL_PALM995)
#  define ATON_MODEL_STR  "palm995 (033_palm_detection_full_quant_pc_uf_od 192x192, ST 995_AI_Hand_Landmarks)"
#  define ATON_MODEL_DECLARE() \
     LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(Default)
#elif defined(CONFIG_AI_ATON_MODEL_EYE)
#  define ATON_MODEL_STR  "eye (blink 64x128 int8, openvela eye controller)"
#  define ATON_MODEL_DECLARE() \
     LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(Default)
#else
#  error "CONFIG_LIB_AI_ATON needs a model (CONFIG_AI_ATON_MODEL_*)"
#endif

/* Does the generated model own a *user allocated output* buffer?
 *
 * 994 (pose) does: LL_ATON_Set_User_Output_Buffer_Default() stores the
 * caller's pointer and the runtime writes the tensor there.
 *
 * 995 (palm detection) does not: its two detector tensors live inside the NPU
 * pool, the generated setter refuses every index with
 * LL_ATON_User_IO_WRONG_INDEX, and the results have to be read from the pool
 * instead.  In that case the runtime still needs the whole span of the
 * declared output tensors, which is what aton_model_output_len() reports.
 */

#if defined(CONFIG_AI_ATON_MODEL_POSE994)
#  define ATON_MODEL_HAS_USER_OUTPUT 1
#else
#  define ATON_MODEL_HAS_USER_OUTPUT 0
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Instantiate the network interface + execution instance for this model.
 * The macro declares the prototypes of the generated symbols and defines a
 * file-local NN_Instance_<name> object filled with the generated pointers.
 */

ATON_MODEL_DECLARE();

/* Face model (YuNet 256x416 int8): the first stage of the two-stage eye
 * pipeline.  Its generated symbols carry the _Face suffix while the eye
 * model's carry _Default, so the two link into one image without clashing.
 *
 * Declaring the interface here is also what keeps the face product linked:
 * every one of its generated symbols appears in NN_Interface_Face's
 * initializer, so --gc-sections cannot drop the object - and with it the
 * epoch program blob it carries in .nsblob.
 */

#if defined(CONFIG_AI_ATON_MODEL_EYE)
LL_ATON_DECLARE_NAMED_NN_INSTANCE_AND_INTERFACE(Face)
#endif

/* Does the primary model own a *user allocated output* buffer?
 *
 * 994 (pose) does: its setter stores the caller's pointer and the runtime
 * writes the tensor there.  995 (palm detection) and both models of the eye
 * build do not: their tensors live inside the NPU pool, the generated setter
 * refuses every index, and the results have to be read from the pool instead
 * - which is what aton_model_output_addr() reports.
 */

#if defined(CONFIG_AI_ATON_MODEL_POSE994)
#  define ATON_MODEL_PRIMARY_USER_OUTPUT 1
#else
#  define ATON_MODEL_PRIMARY_USER_OUTPUT 0
#endif

/* One descriptor per slot.  Everything model specific hangs off it, so a
 * third model is a table entry plus a configuration symbol rather than
 * another set of accessors.
 */

struct aton_model_desc_s
{
  FAR const char          *name;         /* human readable, for the log    */
  FAR NN_Instance_TypeDef *inst;         /* NN_Instance_<suffix>           */
  bool                     user_output;  /* the caller owns the output     */
};

static const struct aton_model_desc_s g_aton_model[ATON_MODEL_NMODELS] =
{
  {
    ATON_MODEL_STR,
    &NN_Instance_Default,
    ATON_MODEL_PRIMARY_USER_OUTPUT != 0
  },
#if defined(CONFIG_AI_ATON_MODEL_EYE)
  {
    "face (YuNet 256x416 int8, ST Edge AI face detection)",
    &NN_Instance_Face,
    false
  },
#endif
};

/* Per-slot inference counters.  The IRQ count, the stage and the verbose flag
 * stay global: there is one NPU and one epoch controller, and the two models
 * run on it one after the other.
 */

static uint32_t g_aton_model_runs[ATON_MODEL_NMODELS];

/* r73/r74: cycle stamps for the phase split printed after every inference.
 * The runtime is NOT cached between inferences (see aton_model_run).
 */

#define ATON_MODEL_DWT_CYCCNT 0xe0001004u

static uint32_t aton_model_cyccnt(void)
{
  return *(FAR volatile uint32_t *)ATON_MODEL_DWT_CYCCNT;
}

/****************************************************************************
 * Name: aton_model_in_info / aton_model_out_info
 *
 * Description:
 *   The generated buffer tables are plain functions with the model suffix
 *   baked into their names, so the dispatch has to be written out.
 *
 ****************************************************************************/

static FAR const LL_Buffer_InfoTypeDef *aton_model_in_info(int id)
{
#if defined(CONFIG_AI_ATON_MODEL_EYE)
  if (id == ATON_MODEL_FACE)
    {
      return LL_ATON_Input_Buffers_Info_Face();
    }
#endif

  (void)id;
  return LL_ATON_Input_Buffers_Info_Default();
}

static FAR const LL_Buffer_InfoTypeDef *aton_model_out_info(int id)
{
#if defined(CONFIG_AI_ATON_MODEL_EYE)
  if (id == ATON_MODEL_FACE)
    {
      return LL_ATON_Output_Buffers_Info_Face();
    }
#endif

  (void)id;
  return LL_ATON_Output_Buffers_Info_Default();
}

/****************************************************************************
 * Name: aton_model_init
 *
 * Description:
 *   Query the model in slot id for its tensor sizes and report what was
 *   linked.  Called once per slot from the ATON driver's session init.
 *
 ****************************************************************************/

int aton_model_init(int id)
{
  uint32_t in_len;
  uint32_t out_len;

  if (id < 0 || id >= ATON_MODEL_NMODELS)
    {
      return -EINVAL;
    }

  in_len  = aton_model_input_len(id);
  out_len = aton_model_output_len(id);

  if (in_len == 0 || out_len == 0)
    {
      _err("ERROR: ATON model %s reports empty tensors (in=%lu out=%lu)\n",
           aton_model_name(id), (unsigned long)in_len,
           (unsigned long)out_len);
      return -EINVAL;
    }

  _info("ATON model: %s, input %lu bytes, output %lu bytes\n",
        aton_model_name(id), (unsigned long)in_len, (unsigned long)out_len);
  return OK;
}

/****************************************************************************
 * Name: aton_model_publish
 *
 * Description:
 *   Publish the model's epoch program: the generated loader reads the blob
 *   out of .nsblob and hands it to the epoch controller.  The loader is a
 *   plain function whose name carries the model suffix, but it is also a
 *   field of the generated NN_Interface_<suffix> object, and that pointer is
 *   what makes this model agnostic.
 *
 * Returned Value:
 *   OK when the loader accepted the program; -EIO when it refused it.
 *
 ****************************************************************************/

int aton_model_publish(int id)
{
  if (id < 0 || id >= ATON_MODEL_NMODELS)
    {
      return -EINVAL;
    }

  return g_aton_model[id].inst->network->ec_network_init() ? OK : -EIO;
}

FAR const char *aton_model_name(int id)
{
  if (id < 0 || id >= ATON_MODEL_NMODELS)
    {
      return "";
    }

  return g_aton_model[id].name;
}

/****************************************************************************
 * Name: aton_model_blob_sync
 *
 * Description:
 *   Publish the secondary epoch blobs of the model in slot id in non-secure
 *   RAM; returns a checksum over the copies.  The hooks live in the generated
 *   network.c of each model and keep their own names.
 *
 ****************************************************************************/

uint32_t aton_model_blob_sync(int id)
{
#if defined(CONFIG_AI_ATON_MODEL_EYE)
  if (id == ATON_MODEL_FACE)
    {
      return aton_model_face_ecblobs_to_ns();
    }
#endif

  (void)id;
  return aton_model_ecblobs_to_ns();
}

/* A model may publish several user tensors that share one buffer - 995's palm
 * detector has three (two 2016x18 heads and a 2016x1 score tensor) inside a
 * single 298368 byte buffer.  The size the runtime has to be handed is the
 * largest offset_end of the list, not the length of its first entry, which is
 * what LL_Buffer_len() would give (8064 for that model).
 */

static uint32_t aton_model_info_len(FAR const LL_Buffer_InfoTypeDef *info)
{
  uint32_t len = 0;

  while (info != NULL && info->name != NULL)
    {
      /* Only the tensors the application owns: the tables also carry the
       * initializers (is_param) and internal scratch buffers, whose offsets
       * belong to another base address entirely.
       */

      if (info->is_user_allocated != 0 && info->offset_end > len)
        {
          len = info->offset_end;
        }

      info++;
    }

  return len;
}

uint32_t aton_model_input_len(int id)
{
  FAR const LL_Buffer_InfoTypeDef *info = aton_model_in_info(id);
  uint32_t len = aton_model_info_len(info);

  return len != 0 ? len : (uint32_t)LL_Buffer_len(info);
}

/* Span of the output tensors a model keeps inside the NPU pool: they are laid
 * out contiguously, so the region the runtime works with is
 * [min(offset_start), max(offset_end)) relative to the buffer base address.
 * Returns the length and hands back the address of that region; both are 0
 * when the table is empty.
 */

static uint32_t aton_model_info_span(FAR const LL_Buffer_InfoTypeDef *info,
                                     FAR uintptr_t *addr)
{
  uint32_t  min_off = 0;
  uint32_t  max_end = 0;
  uintptr_t base    = 0;
  int       first   = 1;

  while (info != NULL && info->name != NULL)
    {
      if (first || info->offset_start < min_off)
        {
          min_off = info->offset_start;
          base    = info->addr_base.i;
          first   = 0;
        }

      if (info->offset_end > max_end)
        {
          max_end = info->offset_end;
        }

      info++;
    }

  if (first || max_end <= min_off)
    {
      *addr = 0;
      return 0;
    }

  *addr = base + min_off;

  /* The buffer tables carry the physical address of the NPU pool
   * (0x34xx_xxxx), but that is not a view either side may use: RISAF gives
   * the pool to the non-secure domain, whose alias is 0x24xx_xxxx.  Two views
   * of the same RAM would also land in two different D-cache lines, so an
   * invalidate issued through one of them leaves the other one stale - the
   * address handed to the application has to be the one everything else uses.
   */

  if (*addr >= 0x34000000u && *addr < 0x34400000u)
    {
      *addr -= 0x10000000u;
    }

  return max_end - min_off;
}

uint32_t aton_model_output_len(int id)
{
  FAR const LL_Buffer_InfoTypeDef *info = aton_model_out_info(id);
  uintptr_t addr;
  uint32_t  len = aton_model_info_len(info);

  if (len != 0)
    {
      return len;                     /* the caller owns the output buffer */
    }

  /* No user allocated output: report the region inside the NPU pool.  This is
   * the size the runtime and the cache maintenance have to cover; 995's two
   * detector tensors span 153216 bytes (298368 - 145152) that way.
   */

  len = aton_model_info_span(info, &addr);

  return len != 0 ? len : (uint32_t)LL_Buffer_len(info);
}

uintptr_t aton_model_output_addr(int id)
{
  FAR const LL_Buffer_InfoTypeDef *info = aton_model_out_info(id);
  uintptr_t addr = 0;

  if (id >= 0 && id < ATON_MODEL_NMODELS && g_aton_model[id].user_output)
    {
      return 0;                       /* the application provides it */
    }

  (void)aton_model_info_span(info, &addr);

  return addr;
}

/* Where the model would read its input from if the application did not
 * register a buffer: the address the input tensor table points at (inside the
 * NPU pool).  Printing it next to the registered pointer tells whether the
 * "user input buffer" handover really happened - if the runtime silently kept
 * its own buffer, the frame never enters the network and every run returns the
 * same bytes.
 */

uintptr_t aton_model_input_addr(int id)
{
  FAR const LL_Buffer_InfoTypeDef *info = aton_model_in_info(id);
  uintptr_t addr = 0;

  (void)aton_model_info_span(info, &addr);
  return addr;
}

/* The epoch program carries the input address as a relocation entry that is
 * resolved once, when the program is published.  Registering a buffer after
 * that has no effect on the engines, so this has to happen before the loader
 * runs; the setter enforces 32-byte alignment and the size.
 */

int aton_model_set_input_buffer(int id, FAR void *buffer, uint32_t size)
{
  LL_ATON_User_IO_Result_t result;

  if (id < 0 || id >= ATON_MODEL_NMODELS)
    {
      return -EINVAL;
    }

  result = g_aton_model[id].inst->network->input_setter(0, buffer, size);
  return result == LL_ATON_User_IO_NOERROR ? OK : -EINVAL;
}

/* What the program would use: the *value* of the user-input pointer, not the
 * address of the pointer variable the buffer table stores.
 */

uintptr_t aton_model_input_pointer(int id)
{
  if (id < 0 || id >= ATON_MODEL_NMODELS)
    {
      return 0;
    }

  return (uintptr_t)g_aton_model[id].inst->network->input_getter(0);
}

uintptr_t aton_model_input_registered(int id)
{
  FAR const LL_Buffer_InfoTypeDef *info = aton_model_in_info(id);

  return info != NULL ? info->addr_base.i : 0;
}

uint32_t aton_model_runs(int id)
{
  if (id < 0 || id >= ATON_MODEL_NMODELS)
    {
      return 0;
    }

  return g_aton_model_runs[id];
}

uint32_t aton_model_irq_count(void)
{
  return aton_osal_nuttx_event_count();
}

int aton_model_run(int id, FAR const void *input, FAR void *output,
                   FAR uint32_t *usec)
{
  FAR NN_Instance_TypeDef *inst;
  LL_ATON_User_IO_Result_t result;
  uint32_t  in_len;
  uint32_t  out_len;
  uintptr_t out_addr;
  uintptr_t out_ptr;
  clock_t   start;
  clock_t   elapsed;
  int       ret = OK;
  uint32_t  c0;                     /* DWT stamps: entry                */
  uint32_t  c1;                     /* ... after RuntimeInit            */
  uint32_t  c2;                     /* ... after Init_Network           */
  uint32_t  c3;                     /* ... after the epoch loop         */
  LL_ATON_RT_RetValues_t ll_aton_rt_ret;

  if (id < 0 || id >= ATON_MODEL_NMODELS)
    {
      return -EINVAL;
    }

  if (input == NULL || output == NULL)
    {
      return -EINVAL;
    }

  inst = g_aton_model[id].inst;

  in_len  = aton_model_input_len(id);
  out_len = aton_model_output_len(id);

  /* A model without a user allocated output (995 and both models of the eye
   * build) writes straight into the NPU pool; the buffer the caller passed in
   * is then only a fallback, and the driver reports the real address through
   * the status ioctl.
   */

  out_addr = aton_model_output_addr(id);
  out_ptr  = out_addr != 0 ? out_addr : (uintptr_t)output;

  /* Make sure the NPU sees the input the CPU produced, and drop any stale
   * (dirty) output lines so the NPU's writes are not overwritten by a later
   * CPU cache write-back.
   */

  up_clean_dcache((uintptr_t)input, (uintptr_t)input + in_len);
  up_invalidate_dcache(out_ptr, out_ptr + out_len);

  result = inst->network->input_setter(0, (FAR void *)input, in_len);
  if (result != LL_ATON_User_IO_NOERROR)
    {
      _err("ERROR: ATON input buffer rejected (result %d)\n", (int)result);
      return -EINVAL;
    }

  if (out_addr == 0)
    {
      result = inst->network->output_setter(0, output, out_len);
      if (result != LL_ATON_User_IO_NOERROR)
        {
          _err("ERROR: ATON output buffer rejected (result %d)\n",
               (int)result);
          return -EINVAL;
        }
    }

  start = clock_systime_ticks();
  c0 = aton_model_cyccnt();

  /* The whole sequence runs on every inference, exactly like
   * LL_ATON_RT_Main() does.  r74 tried to keep the runtime up between
   * inferences and the second call returned without producing any output at
   * all (all-zero tensor, "input ignored" in the fill sweep): the epoch
   * controller needs the init/de-init pair, so this is a sequence, not a
   * state to be cached.  What r74 did prove is where the time goes - see the
   * split printed below.
   */

  LL_ATON_RT_RuntimeInit();
  c1 = aton_model_cyccnt();
  LL_ATON_RT_Init_Network(inst);
  c2 = aton_model_cyccnt();

  do
    {
      ll_aton_rt_ret = LL_ATON_RT_RunEpochBlock(inst);
      if (ll_aton_rt_ret == LL_ATON_RT_WFE)
        {
          LL_ATON_OSAL_WFE();
        }
    }
  while (ll_aton_rt_ret != LL_ATON_RT_DONE);

  c3 = aton_model_cyccnt();

  LL_ATON_RT_DeInit_Network(inst);
  LL_ATON_RT_RuntimeDeInit();

  elapsed = clock_systime_ticks() - start;
  ret = OK;

  _info("ATON run: runtime-init %lu cycles, net-init %lu cycles, epoch %lu "
        "cycles, de-init %lu cycles\n",
        (unsigned long)(c1 - c0),
        (unsigned long)(c2 - c1),
        (unsigned long)(c3 - c2),
        (unsigned long)(aton_model_cyccnt() - c3));

  /* Re-read the results the NPU wrote into the (cacheable) output buffer. */

  up_invalidate_dcache(out_ptr, out_ptr + out_len);

  g_aton_model_runs[id]++;

  if (usec != NULL)
    {
      *usec = (uint32_t)TICK2USEC(elapsed);
    }

  return ret;
}

/****************************************************************************
 * Name: aton_model_dump_log
 *
 * Description:
 *   Print the ATON runtime's ISR-side log (epoch error details).  Task
 *   context only.
 *
 ****************************************************************************/

void aton_model_dump_log(void)
{
  aton_osal_nuttx_log_dump();
}

/****************************************************************************
 * Name: aton_model_report / aton_model_storm_masked
 *
 * Description:
 *   Runtime status for the watchdog: last phase plus counters, and whether
 *   the NPU interrupt line had to be masked because it kept re-asserting.
 *
 ****************************************************************************/

void aton_model_report(FAR const char *tag)
{
  aton_osal_nuttx_report(tag);
}

void aton_model_set_verbose(bool verbose)
{
  aton_osal_nuttx_set_verbose(verbose);
}

bool aton_model_storm_masked(void)
{
  return aton_osal_nuttx_storm();
}

int aton_model_stage(void)
{
  return aton_osal_nuttx_stage();
}

