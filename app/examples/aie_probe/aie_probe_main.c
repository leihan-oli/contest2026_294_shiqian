/****************************************************************************
 * apps/examples/aie_probe/aie_probe_main.c
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

/* Fallback addresses for the tensors, used only when the driver reports none.
 *
 * The input one is not free to choose: the epoch program resolved its input
 * address once, when it was published, and that address is
 * STM32N6_ATON_APP_INPUT_BASE in arch/arm/src/stm32n6/stm32n6_aton_aie.c.
 * Filling any other buffer feeds the engines nothing - the run completes and
 * returns a result that is the same for every frame.  The driver hands the
 * address out through STM32N6_ATON_CMD_GET_STATUS, which is what main() uses;
 * this define only has to stay in step with the driver's.
 *
 * The window itself is the free tail of the NPU's own RAM: the .nsblob
 * section (the epoch program) starts at 0x24328000 and the current model ends
 * at 0x24352f20, so both buffers sit above it and below 0x243c0000.
 */

#define AIE_PROBE_NS_INPUT_ADDR   0x243a2000
#define AIE_PROBE_NS_OUTPUT_ADDR  0x24370000

#include <nuttx/config.h>

#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>

#include <nuttx/arch.h>            /* up_clean_dcache */
#include <nuttx/aie/ai_engine.h>
#include <nuttx/aie/stm32n6_aton_aie.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define AIE_PROBE_MAX_RUNS  16

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Diagnostic output that does not depend on the console driver.
 *
 * The console is written through a TX ring buffer whose wakeups go through a
 * semaphore; if that handshake ever stalls (a writer can be left waiting with
 * an empty buffer - the upper half's own comments flag that window), every
 * task that prints through the driver blocks forever while the CPU is fine.
 * Formatting here and pushing the string to the driver for a polled write
 * keeps the diagnostics flowing in exactly that situation.
 */

static int g_probe_fd = -1;

static void probe_out(FAR const char *fmt, ...)
{
  char    buf[208];
  va_list ap;
  int     n;

  va_start(ap, fmt);
  n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (n <= 0)
    {
      return;
    }

  if (g_probe_fd >= 0)
    {
      (void)ioctl(g_probe_fd, STM32N6_ATON_CMD_PUTS, (unsigned long)buf);
    }
  else
    {
      fprintf(stderr, "%s", buf);
    }
}

/* Progress marker that survives a panic.
 *
 * Console output is interrupt driven: whatever is still queued in the UART TX
 * ring when a fatal fault hits is never transmitted, because the panic path
 * keeps interrupts disabled while it writes its own output with polled writes
 * (which is why panic text can even overtake text that was queued earlier).
 * Waiting for the ring to drain after every marker makes the last marker seen
 * on the terminal the true last step reached - without it a crash says nothing
 * about how far the run got.
 */

static void probe_step(FAR const char *what)
{
  probe_out("aie_probe: [step] %s\n", what);
  fflush(stdout);
  usleep(30000);
}

/* Cheap, stable fingerprint of a buffer: sum, xor and byte min/max.  Used
 * to compare two runs (determinism) and two different inputs (sensitivity)
 * without having to dump hundreds of kilobytes.
 */

static void probe_checksum(FAR const uint8_t *buf, size_t len,
                           FAR uint32_t *sum, FAR uint32_t *xsum,
                           FAR uint8_t *minv, FAR uint8_t *maxv)
{
  uint32_t s = 0;
  uint32_t x = 0;
  uint8_t  lo = 0xff;
  uint8_t  hi = 0x00;
  size_t   i;

  for (i = 0; i < len; i++)
    {
      s += buf[i];
      x ^= ((uint32_t)buf[i] << ((i & 3) * 8));
      if (buf[i] < lo) lo = buf[i];
      if (buf[i] > hi) hi = buf[i];
    }

  *sum  = s;
  *xsum = x;
  *minv = lo;
  *maxv = hi;
}

/* Fill the input tensor with a deterministic pattern.
 *
 *   pattern 0: staircase (0,1,2,...) - exercises the whole value range
 *   pattern 1: checkerboard with a moving phase
 *
 * A real image can be fed instead by passing a file name on the command
 * line (raw bytes, headerless).
 */

static void probe_fill_pattern(FAR uint8_t *buf, size_t len, int pattern)
{
  size_t i;

  for (i = 0; i < len; i++)
    {
      if (pattern == 0)
        {
          buf[i] = (uint8_t)(i & 0xff);
        }
      else
        {
          buf[i] = (uint8_t)((((i / 32) & 1) ^ ((i / 4) & 1)) ? 0xf0 : 0x10);
        }
    }
}

static int probe_fill_from_file(FAR uint8_t *buf, size_t len,
                                FAR const char *path)
{
  ssize_t n;
  int fd;

  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      return -errno;
    }

  n = read(fd, buf, len);
  close(fd);

  if (n < 0)
    {
      return -errno;
    }

  probe_out("aie_probe: input from %s (%zd of %zu bytes)\n", path, n, len);
  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct stm32n6_aton_invoke_params params;
  struct stm32n6_aton_status_s status;
  FAR const char *devpath = CONFIG_EXAMPLES_AIE_PROBE_DEVPATH;
  FAR uint8_t *input;
  FAR uint8_t *output;
  FAR const char *infile = NULL;
  uint32_t in_ck, out_ck;
  uint32_t in_ck_alt, out_ck_alt;
  uint32_t sum, xsum;
  uint32_t total_usec = 0;
  uint8_t  minv, maxv;
  int runs = 1;
  int ret;
  int fd;
  int i;

  if (argc > 1)
    {
      runs = atoi(argv[1]);
      if (runs < 1)
        {
          runs = 1;
        }
      else if (runs > AIE_PROBE_MAX_RUNS)
        {
          runs = AIE_PROBE_MAX_RUNS;
        }
    }

  if (argc > 2)
    {
      infile = argv[2];
    }

  /* 1. Open the AI Engine device and start a session */

  probe_step("open /dev/aie0");

  fd = open(devpath, O_RDONLY);
  if (fd < 0)
    {
      probe_out("aie_probe: open %s failed: %d\n", devpath, errno);
      return 1;
    }

  g_probe_fd = fd;

  probe_out("aie_probe: opened %s\n", devpath);

  probe_step("AIE_CMD_LOAD (aton_model_init)");

  ret = ioctl(fd, AIE_CMD_LOAD, 0);
  if (ret < 0)
    {
      probe_out("aie_probe: AIE_CMD_LOAD failed: %d\n", errno);
      close(fd);
      return 1;
    }

  probe_out("aie_probe: session open (AIE_CMD_LOAD OK)\n");

  /* 2. Ask the driver for the model tensor sizes */

  probe_step("STM32N6_ATON_CMD_GET_STATUS");

  memset(&status, 0, sizeof(status));
  ret = ioctl(fd, STM32N6_ATON_CMD_GET_STATUS, (unsigned long)&status);
  if (ret < 0)
    {
      probe_out("aie_probe: GET_STATUS failed: %d\n", errno);
      close(fd);
      return 1;
    }

  probe_out("aie_probe: model input %lu bytes, output %lu bytes\n",
         (unsigned long)status.input_size,
         (unsigned long)status.output_size);

  if (status.input_size == 0 || status.output_size == 0)
    {
      probe_out("aie_probe: no model linked (input/output size 0)\n");
      close(fd);
      return 1;
    }

  /* 3. Place the tensors in the ATON non-secure window.
   *
   *    The NPU is a non-secure master: it can only read or write memory that
   *    a RISAF region marks non-secure, and the CPU can only reach such a
   *    region through the non-secure alias of the RAM (0x24xx_xxxx, the
   *    secure alias being 0x34xx_xxxx).  Putting the tensors here is what
   *    makes them visible to both sides - a buffer taken from the heap would
   *    live in a secure region and the NPU's accesses to it would be denied.
   *
   *    The window is the free tail of the NPU's own RAM (SRAM3..6 through
   *    its alias): the model only addresses up to 0x2420_0000 + 0xE2480, the
   *    RISAF instances that guard the CPU heap (RISAF2/3) do not cover it,
   *    and the epoch program buffers sit at its base (see .nsblob).
   *
   *    The ATON runtime also requires cache-line aligned buffers
   *    (LL_ATON_Set_User_*_Buffer returns WRONG_ALIGN otherwise).
   */

  probe_step("place input/output tensors in the non-secure window");

  /* The engines read the input from the address the epoch program was
   * published with, so the buffer has to be that one - ask the driver instead
   * of assuming it.  A different address is not an error anywhere: the run
   * completes, the result is simply the same for every frame.
   */

  input = status.input_addr != 0 ?
          (FAR uint8_t *)(uintptr_t)status.input_addr :
          (FAR uint8_t *)AIE_PROBE_NS_INPUT_ADDR;

  /* A model that does not take an output buffer from the caller keeps its
   * result inside the NPU pool and the driver reports where: the hard coded
   * window below only applies when the model has no such tensor (the caller
   * then owns the buffer).  Reading the hard coded address for a model owned
   * output returns whatever happens to live there.
   */

  output = status.output_addr != 0 ?
           (FAR uint8_t *)(uintptr_t)status.output_addr :
           (FAR uint8_t *)AIE_PROBE_NS_OUTPUT_ADDR;

  probe_out("aie_probe: input tensor at 0x%08lx (%s), output at 0x%08lx (%s)\n",
         (unsigned long)(uintptr_t)input,
         status.input_addr != 0 ? "driver registered" : "fallback",
         (unsigned long)(uintptr_t)output,
         status.output_addr != 0 ? "model owned" : "caller owned");

  memset(input, 0, status.input_size);
  memset(output, 0, status.output_size);

  /* Push the freshly written pattern out of the CPU data cache and drop the
   * lines, so that the checksum below reads the pattern back from RAM rather
   * than from the cache.  The tensors live in non-secure memory: a write that
   * a RISAF region denies is dropped silently (no fault), and only a RAM-side
   * check can see that - the checksum used to report all zeros that way.
   */

  up_clean_dcache((uintptr_t)input, (uintptr_t)input + status.input_size);
  up_invalidate_dcache((uintptr_t)input, (uintptr_t)input + status.input_size);

  probe_step("fill input pattern + checksum (verified against RAM)");

  if (infile != NULL)
    {
      ret = probe_fill_from_file(input, status.input_size, infile);
      if (ret < 0)
        {
          probe_out("aie_probe: cannot read %s: %d\n", infile, -ret);
        }
    }
  else
    {
      probe_fill_pattern(input, status.input_size, 0);
    }

  probe_checksum(input, status.input_size, &in_ck, &sum, &minv, &maxv);
  probe_out("aie_probe: input checksum 0x%08lx (min %u max %u)\n",
         (unsigned long)in_ck, (unsigned)minv, (unsigned)maxv);

  /* 3b. Sanity check on the model data.  The ATON model keeps its
   *     initializers (weights and quantization parameters) in the external NOR
   *     at 0x70200000, which has to be programmed from the ST example's
   *     network-data.hex.  Erased flash reads back as 0xff, and the NPU would
   *     read garbage tensor parameters - so check it before blaming the driver.
   *
   *     This is also the first CPU access to the XSPI memory-mapped window, so
   *     it is walked one step at a time: if the XSPI controller is not in
   *     memory-mapped mode the access itself faults.
   */

  {
    /* r34: this used to read the first four bytes of the NOR and sum the first
     * four kilobytes, to tell "weights missing" from "driver broken".  Since
     * r33 that read is no longer possible from the CPU: the NPU needs the same
     * bytes through a non-secure RISAF window, and a RISAF region serves one
     * security domain only, so the secure CPU view reads back as zeros.
     *
     * The equivalent check now happens in the driver, before that window is
     * opened: the boot log line "[nor] cpu pre :" carries the real first words
     * (0x70200000 should read 0xeccebbea for the ST 994 network data).
     */

    probe_step("NOR probe: weights are read by the NPU only");

    probe_out("aie_probe: NOR window belongs to the NPU (see [nor] cpu pre/post in the log)\n");
  }

  /* 4. Run the network.  One AIE_CMD_FEED_INPUT executes the whole epoch
   *    loop on the NPU; the output lands in our buffer.
   */

  probe_step("inference run loop");

  for (i = 0; i < runs; i++)
    {
      memset(&params, 0, sizeof(params));
      params.input       = input;
      params.output      = output;
      params.input_size  = status.input_size;
      params.output_size = status.output_size;

      probe_out("aie_probe: [step] ioctl AIE_CMD_FEED_INPUT (%d)\n", i + 1);
      fflush(stdout);
      usleep(30000);

      ret = ioctl(fd, AIE_CMD_FEED_INPUT, (unsigned long)&params);
      if (ret < 0)
        {
          probe_out("aie_probe: inference %d failed: %d (errno %d)\n",
                  i, ret, errno);
          break;
        }

      total_usec += params.usec;
      probe_out("aie_probe: run %d/%d ok: %lu us, %lu events, result %d\n",
             i + 1, runs, (unsigned long)params.usec,
             (unsigned long)params.events, params.result);
    }

  if (i == 0)
    {
      close(fd);
      return 1;
    }

  probe_checksum(output, status.output_size, &out_ck, &xsum, &minv, &maxv);

  probe_out("aie_probe: output checksum 0x%08lx xor 0x%08lx (min %u max %u)\n",
         (unsigned long)out_ck, (unsigned long)xsum,
         (unsigned)minv, (unsigned)maxv);
  probe_out("aie_probe: avg %lu us over %d run(s)\n",
         (unsigned long)(total_usec / (uint32_t)i), i);

  /* 5. Repeatability: same input must give the same output. */

  memset(output, 0, status.output_size);
  memset(&params, 0, sizeof(params));
  params.input       = input;
  params.output      = output;
  params.input_size  = status.input_size;
  params.output_size = status.output_size;

  ret = ioctl(fd, AIE_CMD_FEED_INPUT, (unsigned long)&params);
  if (ret >= 0)
    {
      probe_checksum(output, status.output_size, &out_ck_alt, &sum,
                     &minv, &maxv);
      probe_out("aie_probe: repeat run checksum 0x%08lx -> %s\n",
             (unsigned long)out_ck_alt,
             (out_ck_alt == out_ck) ? "MATCH (deterministic)" : "MISMATCH");
    }

  /* 6. Sensitivity: a different input must give a different output.  A
   *    constant output would mean the NPU is not really consuming the
   *    input tensor (or the epoch did nothing).
   */

  probe_fill_pattern(input, status.input_size, 1);
  probe_checksum(input, status.input_size, &in_ck_alt, &sum, &minv, &maxv);
  memset(output, 0, status.output_size);
  memset(&params, 0, sizeof(params));
  params.input       = input;
  params.output      = output;
  params.input_size  = status.input_size;
  params.output_size = status.output_size;

  ret = ioctl(fd, AIE_CMD_FEED_INPUT, (unsigned long)&params);
  if (ret >= 0)
    {
      probe_checksum(output, status.output_size, &out_ck_alt, &sum,
                     &minv, &maxv);
      probe_out("aie_probe: input 0x%08lx -> output 0x%08lx (%s)\n",
             (unsigned long)in_ck_alt, (unsigned long)out_ck_alt,
             (out_ck_alt != out_ck) ? "responds to input" :
                                      "SAME as before - check model");
    }

  close(fd);

  probe_out("aie_probe: done\n");
  return 0;
}
