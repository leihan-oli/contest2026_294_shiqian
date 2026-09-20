/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_aton_aie.c
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

/* STM32N6 Neural-ART NPU AI Engine lower-half driver.
 *
 * This driver wraps the ST Neural-ART NPU (accessed through the ST ATON
 * runtime) behind the generic NuttX AI Engine upper-half interface
 * (drivers/aie/ai_engine.c).  It follows the same lower-half pattern as
 * the Arm Ethos-U driver (drivers/aie/ethosu/ethosu_lowerhalf.c).
 *
 * Hardware notes (secure world, NuttX runs in the secure domain):
 *   - NPU register base : 0x580E0000  (STM32_NPU_BASE, AHB5 + 0x0C0000)
 *   - NPU cache RAM      : 0x343C0000  (STM32_NPU_CACHEAXIRAM_BASE, 256 KB)
 *   - Interrupts         : STM32_IRQ_NPU0..NPU3 (NVIC 53..56)
 *   - RCC                : AHB5ENR.NPUEN (bit 31) / AHB5RSTR.NPURST (bit 31)
 *
 * The ATON runtime itself is NOT linked yet (the compiled-in model is not
 * available until the model-training stage completes).  The aie_ops
 * callbacks therefore only perform the hardware bring-up and leave the
 * actual stai_* runtime calls as clearly marked TODO stubs.
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/clock.h>
#include <nuttx/kthread.h>
#include <nuttx/sched.h>
#include <nuttx/signal.h>
#include <nuttx/irq.h>
#include <nuttx/kmalloc.h>
#include <nuttx/mutex.h>
#include <nuttx/nuttx.h>

#include <nuttx/aie/ai_engine.h>
#include <nuttx/aie/stm32n6_aton_aie.h>

#ifdef CONFIG_LIB_AI_ATON
#  include <string.h>
#  include <stai.h>
#  include "npu_cache.h"   /* NuttX CACHEAXI glue (libs/ai_aton/nuttx) */
#  include "aton_model.h"  /* model binding (libs/ai_aton/nuttx) */
#endif

#include "arm_internal.h"
#include "stm32n6_cacheaxi.h"
#include "hardware/stm32_aton.h"
#include "hardware/stm32_cacheaxi.h"
#include "hardware/stm32_rcc.h"
#include "hardware/stm32_memorymap.h"

/* RIFSC RIMC helpers (STM32_RIFSC_RIMC_ATTR, RIFSC_RIMC_ATTR_MCID/MSEC/MPRIV)
 * currently live in the DMA2D header -- the same include the FSBL regression
 * probe uses for its RIMC_ATTR read-back.
 */

#include "hardware/stm32_dma2d.h"

/* NOTE: this file deliberately stays on the image-wide -Os.
 *
 * It briefly carried a "#pragma GCC optimize (\"O0\")" to match the r62 build
 * (no optimisation anywhere), but that trapped at boot: the compiler emitted
 * "udf #255" - its trap for an unreachable path - inside
 * stm32n6_aton_ns_setup() and the CPU jumped straight into it.  GCC's own
 * documentation says the optimize attribute/pragma is for debugging only and
 * not for production code, which is exactly what a mixed -Os/-O0 translation
 * unit does here.  The -O0 pin that actually matters - the ll_aton runtime,
 * the OSAL/cache glue and the generated network - lives in libs/ai_aton and
 * is applied there with a plain target_compile_options().
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_STM32N6_ATON_AIE_DEVPATH
#  define CONFIG_STM32N6_ATON_AIE_DEVPATH "/dev/aie0"
#endif

/* RISAF instances guarding the RAM/LCD/XSPI address spaces, and the
 * illegal-access registers used for post-mortem diagnosis: if the NPU (CID1)
 * is not allowed to reach the tensor pool or the NOR holding the model
 * initializers, the fault is recorded here (IASR, write 1 to clear via IACR).
 */

#define STM32N6_ATON_RISAF1_BASE   (STM32_AHB3_BASE + 0x6000)
#define STM32N6_ATON_RISAF2_BASE   (STM32_AHB3_BASE + 0x7000)
#define STM32N6_ATON_RISAF3_BASE   (STM32_AHB3_BASE + 0x8000)
#define STM32N6_ATON_RISAF4_BASE   (STM32_AHB3_BASE + 0x9000)
#define STM32N6_ATON_RISAF5_BASE   (STM32_AHB3_BASE + 0xa000)
#define STM32N6_ATON_RISAF6_BASE   (STM32_AHB3_BASE + 0xb000)
#define STM32N6_ATON_RISAF7_BASE   (STM32_AHB3_BASE + 0xc000)
#define STM32N6_ATON_RISAF8_BASE   (STM32_AHB3_BASE + 0xd000)
#define STM32N6_ATON_RISAF9_BASE   (STM32_AHB3_BASE + 0xe000)
#define STM32N6_ATON_RISAF11_BASE  (STM32_AHB3_BASE + 0x10000) /* XSPI1 */
#define STM32N6_ATON_RISAF12_BASE  (STM32_AHB3_BASE + 0x11000) /* XSPI2 */
#define STM32N6_ATON_RISAF13_BASE  (STM32_AHB3_BASE + 0x12000) /* XSPI3 */
#define STM32N6_ATON_RISAF_CR      0x00
#define STM32N6_ATON_RISAF_IASR    0x08
#define STM32N6_ATON_RISAF_IACR    0x0c

/* RISAF instance map (RM0486 Table 24):
 *
 *   RISAF1  TCMs           RISAF2  CPU AXI RAM0    RISAF3  CPU AXI RAM1
 *   RISAF4  NPU master 0   RISAF5  NPU master 1    RISAF6  CPU master
 *   RISAF7  FLEXMEM        RISAF8  CACHEAXI        RISAF9  VENCRAM
 *   RISAF11 XSPI1          RISAF12 XSPI2 (NOR)     RISAF13 XSPI3
 *
 * RISAF11..13 decide whether the NPU may read the 4 MB of model initializers
 * that live in the xSPI2 NOR window: an address no enabled base region covers
 * falls back to the primary-region rule, fixed to "secure, privileged and
 * CID = 1 only" (RM0486 7.4.4), so a non-secure master reads zeros there.
 *
 * NOTE: an earlier revision claimed RISAF11..14 fault on access.  That was a
 * bug in this driver's probe loop (the instance table had grown to 13 entries
 * while the initializer still had 9, so the loop dereferenced a NULL entry).
 */

#define STM32N6_ATON_NRISAF        12
#define STM32N6_ATON_NXSPI         3

/* Provided by the board linker script (flash.ld): the non-secure blob window
 * that the .nsblob section is placed in.
 */

extern uint8_t _snsblob[];
extern uint8_t _ensblob[];

/* The ATON OSAL lives in libs/ai_aton.  Only the init entry point is called
 * from this file (the runtime's load-time cache hooks need the mutexes to
 * exist before the first blob is touched), so declare it here instead of
 * pulling in the whole OSAL macro header.
 */

void aton_osal_nuttx_init(void);

/* Helpers defined further down that the boot-time probes already use: a real
 * non-secure read (temporarily re-programming an SAU region) and the one-line
 * RISAF latch summary.
 */

static uint32_t stm32n6_aton_ns_read32(int region, uint32_t addr);
static void stm32n6_aton_latch_report(void);

/* SAU registers (ARMv8-M core private region).  Programmed directly: the
 * ARMv8-M port has the helpers in arm_sau.c, but that file is not part of
 * this build.
 */

#define STM32N6_ATON_SAU_CTRL  0xe000edd0
#define STM32N6_ATON_SAU_RNR   0xe000edd8
#define STM32N6_ATON_SAU_RBAR  0xe000eddc
#define STM32N6_ATON_SAU_RLAR  0xe000ede0

/* Value of aton_osal_nuttx_stage() while the ATON runtime sits in its
 * wait-for-NPU loop (ATON_STAGE_WFE on the OSAL side).
 */

#define STM32N6_ATON_STAGE_WFE  5

/* Consecutive watchdog samples with an unchanged BC before the intrusive
 * diagnostic dump is taken: one sample is a healthy wait-for-event, several
 * are a stalled epoch controller.
 */

#define STM32N6_ATON_STALL_SAMPLES  3

/* Program one SAU region as non-secure.  RBAR holds the base, RLAR the
 * inclusive limit with bit 0 as the region enable; both work on a 32 byte
 * granularity, so the low five bits are dropped.
 */

static void stm32n6_aton_sau_ns_region(int region, uint32_t base, uint32_t size)
{
  putreg32((uint32_t)region, STM32N6_ATON_SAU_RNR);
  putreg32(base & 0xffffffe0, STM32N6_ATON_SAU_RBAR);
  putreg32(((base + size - 1) & 0xffffffe0) | 0x1, STM32N6_ATON_SAU_RLAR);
}

/* RISAF illegal-access entry: IAESR (attributes of the rejected master) and
 * IADDR (full 32-bit address of the rejected access).  IASR only carries the
 * flags, so the address must be read here and not reconstructed from IASR.
 */

#define STM32N6_ATON_RISAF_IAESR   0x20
#define STM32N6_ATON_RISAF_IADDR   0x24

/* Region n occupies 0x30 bytes at 0x40 + 0x30 * n.  Field order inside a
 * region block is CFGR, STARTR, ENDR, CIDCFGR (STM32N647xx CMSIS
 * RISAF_Region_TypeDef) - note that CFGR comes first, not last.
 */

#define STM32N6_ATON_RISAF_REG0    0x40

/* Region attribute words.  Bit 0 is BREN (base region enable), bit 8 is SEC
 * ("this region is for secure masters"), bits 16..23 are PRIVC0..7 (the CID
 * mask of masters that are allowed in with privilege).
 *
 *   ..._CFGR_SEC_OPEN: BREN | SEC | every CID privileged (the word the board
 *                      FSBL uses for the RAM windows it opens)
 *   ..._CFGR_NS_OPEN:  BREN |      | every CID privileged, non-secure region
 */

#define STM32N6_ATON_RISAF_CFGR_SEC_OPEN 0x00ff0101
#define STM32N6_ATON_RISAF_CFGR_NS_OPEN  0x00ff0001

/* One region block is 16 words long (CFGR, STARTR, ENDR, CIDCFGR, the A and B
 * sub-region registers and four reserved words), so region n sits at
 * REG0 + 0x40 * n.
 */

#define STM32N6_ATON_RISAF_REGION_STRIDE 0x40

/* Subregion A of a base region (RM0486 7.5.10..7.5.13).  Table 26 allows a
 * secure privileged base region to carry a non-secure subregion, which is the
 * only way one address range can serve both security domains - and the NOR
 * weights need exactly that (see the domain sweep below).
 */

#define STM32N6_ATON_RISAF_REG_ACFGR   0x50
#define STM32N6_ATON_RISAF_REG_ASTARTR 0x54
#define STM32N6_ATON_RISAF_REG_AENDR   0x58
#define STM32N6_ATON_RISAF_REG_ANESTR  0x5c

#define STM32N6_ATON_RISAF_SR_SREN     (1u << 0)
#define STM32N6_ATON_RISAF_SR_SEC      (1u << 8)
#define STM32N6_ATON_RISAF_SR_PRIV     (1u << 9)
#define STM32N6_ATON_RISAF_SR_RDEN     (1u << 12)
#define STM32N6_ATON_RISAF_SR_WREN     (1u << 13)

/* Non-secure alias windows.
 *
 * STM32N6 maps its internal RAMs twice: at 0x3400_0000.. ("secure alias") and
 * at 0x2400_0000.. ("non-secure alias"), the difference between the two being
 * 0x1000_0000.  The NPU is a non-secure master - the RISAF records its
 * transactions as "ns" no matter what the RIMC says about it - so every
 * buffer it touches has to sit in a RISAF region that is configured
 * non-secure, and the CPU can only reach such a region through the non-secure
 * alias (a secure access is not accepted by a non-secure region, which is why
 * the FSBL had to keep its windows secure).
 *
 * Using the alias is therefore what lets both sides share a buffer: the CPU
 * writes through 0x24xx_xxxx (non-secure transaction) and the NPU reads the
 * very same words.
 *
 *   0x2410_0000  SRAM2 alias : .nsblob (epoch program buffers) + user I/O tensors
 *   0x2420_0000  SRAM3..6 alias : NPU tensor pool (model pools 0..3 and 7)
 *   0x243C_0000  NPU cache RAM alias
 */

#define STM32N6_ATON_NS_ALIAS_BASE   0x24000000
#define STM32N6_ATON_NS_ALIAS_SIZE   0x00400000

/* Physical view of the same RAM (SRAM3..6 and the NPU cache RAM).  The RISAF
 * filters by address, so both views are opened: the CPU and the model use the
 * alias (a non-secure transaction by construction) and the NPU may end up
 * presenting either.
 */

#define STM32N6_ATON_NS_PHYS_BASE    0x34200000
#define STM32N6_ATON_NS_PHYS_SIZE    0x00200000

/* The epoch program buffers and the user I/O tensors live in the XSPI1
 * HyperRAM, which no CPU-side code uses and which is inside no address space
 * that also holds CPU data.
 */

#define STM32N6_ATON_NS_HPRAM_BASE   0x90000000
#define STM32N6_ATON_NS_HPRAM_SIZE   0x00400000

/* The blob window: inside the NPU RAM non-secure window (the only one RISAF
 * lets a non-secure master and the SAU-marked CPU accesses through), but above
 * the region the model's tensors actually use - max(offset_end) over its
 * tables.  SRAM2 is not an option: RISAF2/3 keep it secure-only, so NS writes
 * there are denied silently, and the kernel's own .bss lives in it.
 */

/* The application's input buffer (see apps/examples/palm_cam).  The model's
 * input pointer has to be registered with this address before the epoch
 * program is published.
 */

/* Size of the .nsblob section (the epoch program and its secondary blobs as
 * the linker placed them in the non-secure window).  It is the amount of
 * memory that has to be cleaned out of the CPU data cache before the NPU
 * fetches the program.
 */

/* NOTE: this used to be a hardcoded 0x7620, which happens to be palm995's
 * blob size - a leftover from a single-model build.  With two models linked
 * in, the section holds both blobs, so derive it from the linker symbols.
 */

#define STM32N6_ATON_NS_BLOB_SIZE     ((uint32_t)((uintptr_t)_ensblob - \
                                                  (uintptr_t)_snsblob))

/* RIMC master attribute register 1: the security attributes of the ATON
 * (NPU) master.  MSEC (bit 8) | MPRIV (bit 9) | MCID (bits 6:4) = CID 1.
 */

#define STM32N6_ATON_RIMC_ATTR1       0x54024c14

/* The application's input buffer.  A model's input pointer has to be
 * registered with this address before its epoch program is published.
 *
 * Both slots of a two-model build share it.  The non-secure window is 608 KB
 * (0x24328000..0x243c0000) and the two .nsblob segments take 224 KB of that,
 * which leaves 384 KB - not enough for two tensors side by side, since the eye
 * classifier wants 24 KB and the face detector 312 KB.  Sharing is safe
 * because the application runs the models in sequence over one frame: the face
 * tensor is dead the moment its inference returns, and the eye crop is taken
 * from the camera frame, never from what the face network was fed.
 *
 * The address sits past the blobs: .nsblob runs from 0x24328000 to about
 * 0x24369210 once the face model's two SW-epoch programs are in it, and
 * 0x2436c000 + 0x4e000 = 0x243ba000 stays below the end of the window at
 * 0x243c0000 with 24 KB to spare.  Mind the order - 94% of the window is
 * spoken for, and the blobs growing into the buffer would corrupt both
 * silently.  stm32n6_aton_aie_init() checks the two against each other.
 *
 * Keep the size in step with the largest model input:
 * stm32n6_aton_publish_model() refuses a model whose tensor does not fit.
 */

#define STM32N6_ATON_APP_INPUT_BASE  0x2436c000
#define STM32N6_ATON_APP_INPUT_SIZE  0x4e000     /* 319488 = 1x3x256x416 i8 */

#define STM32N6_ATON_NS_WINDOW_BASE  0x24328000
/* 608 KB: widened from 144 KB (0x24000) on 2026-09-17 for the eye model,
 * whose epoch program is 172 KB.  flash.ld documents
 * 0x24328000..0x243C0000 as free, hence LENGTH = 0x243C0000 - 0x24328000.
 * The two values must stay in sync.
 */

#define STM32N6_ATON_NS_WINDOW_SIZE  0x00098000

/* Read and write enable for every CID (RDENC0..7 | WRENC0..7 << 16). */

#define STM32N6_ATON_RISAF_CIDCFGR_OPEN  0x00ff00ff

/* Whole address space of the instance (STARTR/ENDR are offsets inside the
 * space that the RISAF instance protects).
 *
 * RISAF4/RISAF5/RISAF6 report a 4 GB space, but 0xffffffff must not be used:
 * the end address is compared as "addr < limit + 1", so 0xffffffff overflows
 * to 0 and every access is rejected (a mistake already made in the FSBL, see
 * stm32n6-rif-fsbl notes: "EndAddress=0xFFFFFFFF will fail the 32-bit
 * overflow assert -> use 0x3FFFFFFF").  0x3FFFFFFF covers the 0x34000000
 * based AXISRAM windows with room to spare.  RISAF8 guards the 256 KB NPU
 * CACHEAXI RAM, whose published limit is 0x0003ffff.
 */

#define STM32N6_ATON_RISAF4_6_END        0x3fffffff
#define STM32N6_ATON_RISAF8_END          0x0003ffff

/****************************************************************************
 * Private Types
 ****************************************************************************/

/* Per-slot state.  Tensors, buffers and counters belong to a model; only
 * the IRQ count and the epoch-event count stay global, because there is one
 * NPU and one epoch controller, and the models run on it one after the other.
 */

struct stm32n6_aton_model_s
{
  uint32_t runs;        /* Inferences this slot completed              */
  uint32_t last_usec;   /* Duration of its last inference              */
  int      last_result; /* Result of its last inference                */
  uint32_t input_addr;  /* Address its program was published with      */
  uint32_t input_size;  /* Its input tensor size in bytes              */
  uint32_t output_addr; /* Where its output lives inside the NPU pool  */
  uint32_t output_size; /* Span of its output tensors in bytes         */
  bool     published;   /* The epoch controller accepted its program   */
  bool     warned;      /* The "fed the wrong buffer" note was printed */
};

struct stm32n6_aton_lowerhalf_s
{
  struct aie_lowerhalf_s lower;  /* Common lower-half interface (ops first) */

  mutex_t                lock;   /* Serialise access to the NPU */
  bool                   opened; /* Session (AIE_CMD_LOAD) active */
  uint32_t               irq_count;   /* Stray NPU interrupts (diagnostics) */
  uint32_t               events;      /* NPU epoch-complete events */

  struct stm32n6_aton_model_s model[ATON_MODEL_NMODELS];
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static FAR void *stm32n6_aton_aie_init(FAR struct aie_lowerhalf_s *lower,
                                       uintptr_t model);
static int stm32n6_aton_aie_deinit(FAR struct aie_lowerhalf_s *lower,
                                   FAR void *context);
static int stm32n6_aton_aie_feed_input(FAR struct aie_lowerhalf_s *lower,
                                       FAR void *context, uintptr_t input);
static int stm32n6_aton_aie_get_output(FAR struct aie_lowerhalf_s *lower,
                                       FAR void *context, uintptr_t output);
static int stm32n6_aton_aie_control(FAR struct aie_lowerhalf_s *lower,
                                    FAR void *context, int cmd,
                                    unsigned long arg);

/* NPU interrupt handler (one per NVIC line; all forward to a helper) */

static int stm32n6_aton_npu_irq(int irq, FAR void *context, FAR void *arg);

/* Inference watchdog: reports progress and breaks interrupt storms */

static int stm32n6_aton_watchdog(int argc, FAR char *argv[]);

/* Raw (polled, unbuffered) output used by the heartbeat */

static void stm32n6_aton_wd_puts(FAR const char *str);
static void stm32n6_aton_wd_u32(uint32_t value);
static void stm32n6_aton_wd_hex(uint32_t value);
static void stm32n6_aton_risaf_clear(void);
static void stm32n6_aton_weights_check(void);
static void stm32n6_aton_matrix_probe(void);
static void stm32n6_aton_npu_clocks_on(void);

/* Console transmitter state (stm32n6_serial.c): printed with raw output so it
 * is visible even when the console driver itself is what is stuck.
 */

void stm32n6_console_dump(void);

/* Recover a lost console TX wakeup (see stm32n6_serial.c) */

void stm32n6_console_unstick(void);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Set while an inference is in flight, read by the watchdog thread */

static volatile bool g_aton_run_active;
static bool          g_aton_wd_started;

/* The watchdog must never compete with the application: it runs at the work
 * queue priority (below every user task), so even if its sleep ever returned
 * early it could not starve anything.
 */

#define ATON_WD_PRIORITY   224
#define ATON_WD_STACKSIZE  4096
#define ATON_WD_PERIOD_US  (2 * 1000 * 1000)

static const struct aie_ops_s g_stm32n6_aton_aie_ops =
{
  .init       = stm32n6_aton_aie_init,
  .deinit     = stm32n6_aton_aie_deinit,
  .feed_input = stm32n6_aton_aie_feed_input,
  .get_output = stm32n6_aton_aie_get_output,
  .control    = stm32n6_aton_aie_control,
};

/* NVIC interrupt numbers handled by this driver */

static const int g_stm32n6_aton_npu_irqs[] =
{
  STM32_IRQ_NPU0,
  STM32_IRQ_NPU1,
  STM32_IRQ_NPU2,
  STM32_IRQ_NPU3
};

#define STM32N6_ATON_NIRQS \
  (sizeof(g_stm32n6_aton_npu_irqs) / sizeof(g_stm32n6_aton_npu_irqs[0]))

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_aton_npu_irq
 *
 * Description:
 *   NPU interrupt handler.  The Neural-ART NPU raises up to four interrupt
 *   lines (epoch end + three error lines, aliased to NVIC 53..56).
 *
 *   This handler is installed at boot so that the lines never fall through
 *   to NuttX's "unexpected interrupt" path.  Once the ATON runtime starts
 *   an inference (LL_ATON_RT_Main) it installs its OWN handler for line 0
 *   (ATON_STD_IRQHandler == NPU0_IRQHandler) through the OSAL hook
 *   LL_ATON_OSAL_INSTALL_IRQ(); that handler clears the ATON interrupt
 *   controller status and signals LL_ATON_OSAL_WFE(), so it replaces this
 *   one for the duration of the epoch loop (see ll_aton_osal_nuttx.c).
 *
 *   The handler must stay SILENT: the NSH console (USART1) is interrupt
 *   driven, so a syslog()/_info()/_err() call from interrupt context fights
 *   the task that owns the console and freezes the shell (observed
 *   2026-09-11 right after aie_probe returned).  Enabling
 *   CONFIG_SYSLOG_INTBUFFER to make such prints "safe" is NOT an option on
 *   this board either: it corrupted the NuttX heap (mm.h:354 assert that
 *   reproduces on the first task spawn after boot).
 *
 *   If diagnostics are needed, accumulate state here and expose it from
 *   task context (ioctl) instead of printing.
 *
 ****************************************************************************/

static int stm32n6_aton_npu_irq(int irq, FAR void *context, FAR void *arg)
{
  FAR struct stm32n6_aton_lowerhalf_s *priv =
    (FAR struct stm32n6_aton_lowerhalf_s *)arg;

  UNUSED(irq);
  UNUSED(context);

  /* Count stray interrupts for post-mortem diagnostics (task context only).
   * While an inference is running the ATON runtime's handler is installed
   * instead of this one, so this counter only sees interrupts raised while
   * no session is active.
   */

  if (priv != NULL)
    {
      priv->irq_count++;
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_aton_rif_fault_dump
 *
 * Description:
 *   Report and clear any RISAF illegal-access records.  Called after a failed
 *   inference: a bus error seen by the NPU (e.g. reading the model weights in
 *   the external NOR or the tensor pool in AXISRAM3..6) shows up here as a
 *   non-zero IASR of the region that rejected the access.
 *
 ****************************************************************************/

static void stm32n6_aton_rif_fault_dump(void)
{
  static const uint32_t risaf[STM32N6_ATON_NRISAF] =
  {
    STM32N6_ATON_RISAF1_BASE, STM32N6_ATON_RISAF2_BASE,
    STM32N6_ATON_RISAF3_BASE, STM32N6_ATON_RISAF4_BASE,
    STM32N6_ATON_RISAF5_BASE, STM32N6_ATON_RISAF6_BASE,
    STM32N6_ATON_RISAF7_BASE, STM32N6_ATON_RISAF8_BASE,
    STM32N6_ATON_RISAF9_BASE, STM32N6_ATON_RISAF11_BASE,
    STM32N6_ATON_RISAF12_BASE, STM32N6_ATON_RISAF13_BASE
  };

  int i;

  for (i = 0; i < STM32N6_ATON_NRISAF; i++)
    {
      uint32_t iasr  = getreg32(risaf[i] + STM32N6_ATON_RISAF_IASR);
      uint32_t iaesr;
      uint32_t iaddr;

      if (iasr == 0)
        {
          continue;
        }

      /* IASR only carries the flags; the failing master and the address live
       * in the per-entry registers (RISAF_IAESR = base + 0x20,
       * RISAF_IADDR = base + 0x24):
       *
       *   IAESR[2:0] = IACID   (compartment id of the offending master)
       *   IAESR[4]   = IAPRIV  (master was privileged)
       *   IAESR[5]   = IASEC   (master was secure)
       *   IAESR[7]   = IANRW   (1 = write, 0 = read)
       */

      iaesr = getreg32(risaf[i] + STM32N6_ATON_RISAF_IAESR);
      iaddr = getreg32(risaf[i] + STM32N6_ATON_RISAF_IADDR);

      stm32n6_aton_wd_puts("[risaf-fault] risaf");
      stm32n6_aton_wd_u32(i + 1);
      stm32n6_aton_wd_puts(" addr=");
      stm32n6_aton_wd_hex(iaddr);
      stm32n6_aton_wd_puts(" cid=");
      stm32n6_aton_wd_u32(iaesr & 0x7);
      stm32n6_aton_wd_puts((iaesr & 0x20) ? " sec" : " ns");
      stm32n6_aton_wd_puts((iaesr & 0x10) ? " priv" : " unpriv");
      stm32n6_aton_wd_puts((iaesr & 0x80) ? " write" : " read");
      stm32n6_aton_wd_puts(" iaesr=");
      stm32n6_aton_wd_hex(iaesr);
      stm32n6_aton_wd_puts(" iasr=");
      stm32n6_aton_wd_hex(iasr);
      stm32n6_aton_wd_puts("\r\n");

      /* Show the policy of the region that rejected the access: CFGR carries
       * BREN/SEC/PRIVC0..7, CIDCFGR the per-CID read/write enables.
       */

      stm32n6_aton_wd_puts("[risaf-fault] risaf");
      stm32n6_aton_wd_u32(i + 1);
      stm32n6_aton_wd_puts(" region0 cfgr=");
      stm32n6_aton_wd_hex(getreg32(risaf[i] + STM32N6_ATON_RISAF_REG0 + 0x00));
      stm32n6_aton_wd_puts(" start=");
      stm32n6_aton_wd_hex(getreg32(risaf[i] + STM32N6_ATON_RISAF_REG0 + 0x04));
      stm32n6_aton_wd_puts(" end=");
      stm32n6_aton_wd_hex(getreg32(risaf[i] + STM32N6_ATON_RISAF_REG0 + 0x08));
      stm32n6_aton_wd_puts(" cidc=");
      stm32n6_aton_wd_hex(getreg32(risaf[i] + STM32N6_ATON_RISAF_REG0 + 0x0c));
      stm32n6_aton_wd_puts("\r\n");

      putreg32(iasr, risaf[i] + STM32N6_ATON_RISAF_IACR);
    }
}

/****************************************************************************
 * Name: stm32n6_aton_risaf_open
 *
 * Description:
 *   Open the whole address space of one RISAF instance to every compartment
 *   ID, using the same policy word that the board FSBL uses for the windows
 *   it programs (BREN | SEC | all CIDs privileged, read and write enabled for
 *   every CID).
 *
 *   This exists because the FSBL leaves RISAF4/RISAF5/RISAF6 - the blocks
 *   that guard the large AXISRAM windows, i.e. the NPU tensor pool at
 *   0x34200000 (RM0486 Table 24) - completely unprogrammed, and an
 *   unprogrammed RISAF falls back to its reset rule ("secure, privileged and
 *   CID 1 only", RM0486 7.4.3).  Writes to those blocks had to be skipped in
 *   the FSBL because they took the boot down; doing it here, with the RISAF
 *   clock already running, lets the NPU reach its own memory.
 *
 *   Without this the epoch controller cannot fetch its program, which is what
 *   EC_IRQ ERR_START ("wrong blob ID") reports.
 *
 ****************************************************************************/

static void stm32n6_aton_risaf_region(uint32_t base, int region,
                                      uint32_t start, uint32_t end,
                                      uint32_t cfgr)
{
  uint32_t r = base + STM32N6_ATON_RISAF_REG0 +
               (uint32_t)region * STM32N6_ATON_RISAF_REGION_STRIDE;

  /* Program start/end and the CID grants first, enable the region last so
   * that it never matches with half of its attributes in place.
   */

  putreg32(start, r + 0x04);
  putreg32(end, r + 0x08);
  putreg32(STM32N6_ATON_RISAF_CIDCFGR_OPEN, r + 0x0c);
  putreg32(cfgr, r + 0x00);
}

/* Subregion A of base region x: disabled while the addresses change, then
 * enabled with the granted rights.
 */

static void stm32n6_aton_risaf_subregion(uint32_t base, int region,
                                         uint32_t start, uint32_t end,
                                         uint32_t acfgr)
{
  uint32_t r = base + STM32N6_ATON_RISAF_REG0 +
               (uint32_t)region * STM32N6_ATON_RISAF_REGION_STRIDE;

  putreg32(0, r + STM32N6_ATON_RISAF_REG_ACFGR);
  putreg32(start, r + STM32N6_ATON_RISAF_REG_ASTARTR);
  putreg32(end, r + STM32N6_ATON_RISAF_REG_AENDR);
  putreg32(0, r + STM32N6_ATON_RISAF_REG_ANESTR);
  putreg32(acfgr, r + STM32N6_ATON_RISAF_REG_ACFGR);
}

/****************************************************************************
 * Name: stm32n6_aton_ns_setup
 *
 * Description:
 *   Open the non-secure alias windows that the NPU needs, and tell the SAU
 *   that those addresses are non-secure for the CPU as well.
 *
 *   RISAF1 and RISAF4/RISAF5/RISAF6 all describe the whole 4 GB address map
 *   (their address space limit is 0xFFFFFFFF), so the window has to be carved
 *   out of the middle of each of them: region 0 keeps the low part of the map
 *   secure, region 2 keeps the high part secure - the CPU's own code and data
 *   at 0x3400_0000.. live in that one - and region 1 is the non-secure window.
 *
 *   The write order matters: the CPU never touches 0x2400_0000.. before this
 *   function returns, so the extra secure region is installed first, the
 *   non-secure window second and the secure region 0 is narrowed last.  Any
 *   other order would leave an instant in which the CPU finds no region
 *   covering the RAM it is running from, and an unmatched access is denied.
 *
 ****************************************************************************/

static void stm32n6_aton_ns_setup(void)
{
  static const uint32_t instances[4] =
  {
    STM32N6_ATON_RISAF1_BASE, STM32N6_ATON_RISAF4_BASE,
    STM32N6_ATON_RISAF5_BASE, STM32N6_ATON_RISAF6_BASE
  };

  int i;

  for (i = 0; i < 4; i++)
    {
      uint32_t base = instances[i];

      /* Overlap-free layout, written in an order that never leaves the CPU
       * uncovered: the extra secure regions go in while region 0 still spans
       * the whole map, and region 0 is narrowed last.
       *
       *   0: secure     [0x0000_0000, 0x23FF_FFFF]  low map
       *   1: non-secure [0x2400_0000, 0x243F_FFFF]  alias view of SRAM1..6+cache
       *   2: secure     [0x2440_0000, 0x341F_FFFF]  CPU RAM: image, heap, stacks
       *   3: non-secure [0x3420_0000, 0x343F_FFFF]  physical view of NPU RAM
       *   4: secure     [0x3440_0000, 0x3FFF_FFFF]  rest of the map
       */

      stm32n6_aton_risaf_region(base, 2, 0x24400000, 0x341fffff,
                                STM32N6_ATON_RISAF_CFGR_SEC_OPEN);
      stm32n6_aton_risaf_region(base, 4, 0x34400000, 0x3fffffff,
                                STM32N6_ATON_RISAF_CFGR_SEC_OPEN);
      stm32n6_aton_risaf_region(base, 1, STM32N6_ATON_NS_ALIAS_BASE,
                                STM32N6_ATON_NS_ALIAS_BASE +
                                STM32N6_ATON_NS_ALIAS_SIZE - 1,
                                STM32N6_ATON_RISAF_CFGR_NS_OPEN);
      stm32n6_aton_risaf_region(base, 3, STM32N6_ATON_NS_PHYS_BASE,
                                STM32N6_ATON_NS_PHYS_BASE +
                                STM32N6_ATON_NS_PHYS_SIZE - 1,
                                STM32N6_ATON_RISAF_CFGR_NS_OPEN);
      stm32n6_aton_risaf_region(base, 0, 0, 0x23ffffff,
                                STM32N6_ATON_RISAF_CFGR_SEC_OPEN);
    }

  /* RISAF2/RISAF3 are left alone, and no external memory is touched: the
   * non-secure window lives in the free tail of the NPU's own RAM, which
   * only RISAF1/4/5/6 describe.
   */

  /* RISAF2 and RISAF3 are left exactly as the FSBL and r13 had them: region 0
   * secure over their whole one megabyte space.
   *
   * They guard the CPU AXI RAMs - the RAM the NuttX heap lives in - and r14
   * showed what a second, non-secure region there does: the overlapping pair
   * captured the CPU's own secure accesses, the writes were dropped without a
   * fault, the free-list metadata went stale and mm_malloc() tripped on it.
   * Their address space base cannot be pinned down from software either, so
   * there is no safe way to carve them: the non-secure window simply stays
   * out of the RAM they protect.
   */

  /* RISAF8 guards the NPU cache RAM (0x343C_0000, 256 KB).  Region 0 stays at
   * its reset value (disabled) and a single non-secure region covers all of
   * it: the cache carries the NPU's non-secure traffic and nothing else uses
   * that RAM.
   */

  stm32n6_aton_risaf_region(STM32N6_ATON_RISAF8_BASE, 0, 0, 0, 0);
  stm32n6_aton_risaf_region(STM32N6_ATON_RISAF8_BASE, 1, 0, 0x3ffff,
                            STM32N6_ATON_RISAF_CFGR_NS_OPEN);

  /* Mark the window non-secure for the CPU too.  The alias already selects
   * the non-secure domain, but making the SAU say so as well removes any
   * dependence on the IDAU's view of those addresses.
   */

  stm32n6_aton_wd_puts("[ns] sau ctrl 0x");
  stm32n6_aton_wd_hex(getreg32(STM32N6_ATON_SAU_CTRL));
  stm32n6_aton_wd_puts(" -> ");

  /* Disable the SAU while programming it, as the architecture requires.  With
   * the SAU off every address is secure, which is also how the CPU has been
   * running so far, so this is safe.
   */

  putreg32(0, STM32N6_ATON_SAU_CTRL);

  stm32n6_aton_sau_ns_region(0, STM32N6_ATON_NS_ALIAS_BASE,
                             STM32N6_ATON_NS_ALIAS_SIZE);

  /* ENABLE = 1, ALLNS = 0: only the region above is non-secure, everything
   * else keeps its default secure attribute.
   */

  putreg32(0x1, STM32N6_ATON_SAU_CTRL);

  stm32n6_aton_wd_hex(getreg32(STM32N6_ATON_SAU_CTRL));
  stm32n6_aton_wd_puts("\r\n");

  /* The window holds a NOLOAD section, so nothing initialises it at startup:
   * clear it before the epoch buffers are filled in.
   *
   * This is the first access the CPU makes through the non-secure alias, so
   * announce it: if the firmware stops here, the bus error is that access and
   * nothing else.
   */

  stm32n6_aton_wd_puts("[ns] priming npu ram 0x");
  stm32n6_aton_wd_hex(STM32N6_ATON_NS_WINDOW_BASE);
  stm32n6_aton_wd_puts("..0x");
  stm32n6_aton_wd_hex(STM32N6_ATON_NS_WINDOW_BASE +
                      STM32N6_ATON_NS_WINDOW_SIZE);
  stm32n6_aton_wd_puts(" ");

  memset((void *)STM32N6_ATON_NS_WINDOW_BASE, 0,
         STM32N6_ATON_NS_WINDOW_SIZE);

  stm32n6_aton_wd_puts("ok\r\n");

  /* r32: the eleven secondary epoch blobs are copied into this window by the
   * start-up path, and the clear above reaches into the copies (they start at
   * 0x242e4000 and run up to 0x24317b50, so the tail of the window overlaps
   * them).  Take the copies again here, after the clear, so the order cannot
   * matter, and print the checksum: the same value as the earlier copy proves
   * the window is stable.
   */

#ifdef CONFIG_LIB_AI_ATON
  stm32n6_aton_wd_puts("[nsblob] re-copy after clear sum=");
  stm32n6_aton_wd_hex(aton_model_blob_sync(ATON_MODEL_PRIMARY));
  stm32n6_aton_wd_puts("\r\n");
#endif

  stm32n6_aton_wd_puts("[ns] blobs at 0x");
  stm32n6_aton_wd_hex((uint32_t)(uintptr_t)_snsblob);
  stm32n6_aton_wd_puts("..0x");
  stm32n6_aton_wd_hex((uint32_t)(uintptr_t)_ensblob);
  stm32n6_aton_wd_puts(", window 0x");
  stm32n6_aton_wd_hex(STM32N6_ATON_NS_WINDOW_BASE);
  stm32n6_aton_wd_puts("+0x");
  stm32n6_aton_wd_hex(STM32N6_ATON_NS_WINDOW_SIZE);
  stm32n6_aton_wd_puts("\r\n");

  /* Start the model run from a clean slate: anything latched from here on was
   * caused by the NPU (or the probe), not by bring-up.
   */

  stm32n6_aton_risaf_clear();

  /* The NPU's own error latches (one bit per AXI bus port) and the cache
   * error flag: clear them so the run starts from a known state.
   */

  putreg32(getreg32(STM32_ATON_BUSIF0_ERR), STM32_ATON_BUSIF0_ERR);
  putreg32(getreg32(STM32_ATON_BUSIF1_ERR), STM32_ATON_BUSIF1_ERR);
  putreg32(CACHEAXI_FCR_CERRF, STM32_CACHEAXI_FCR);

  stm32n6_aton_wd_puts("[risaf] illegal-access latches cleared\r\n");

  /* Hardware ground truth about what a rejected read looks like. */

  stm32n6_aton_matrix_probe();
}

/****************************************************************************
 * Name: stm32n6_aton_nor_domain_sweep
 *
 * Description:
 *   Decide by measurement which security-domain configuration lets both the
 *   secure and the non-secure readers reach the model weights in the external
 *   NOR.  The two previous rounds proved that both kinds of reader exist:
 *
 *     r51 (window = non-secure only)  secure reader refused
 *                                     (risaf11 addr=0x703246e8, iaesr=0x31)
 *     r52 (window = secure only)      non-secure readers refused
 *                                     (risaf4/5 addr=0x7030b3a0/0x70316700,
 *                                      iaesr=0x11)
 *
 *   A refusal is silent - the read returns zeros without stalling or raising
 *   an error - so either polarity poisons the inference invisibly.  RM0486
 *   7.4.4 makes an address no enabled base region covers secure-only (the
 *   vendor example relies on that reset rule), CFGR.SEC lets one base region
 *   serve exactly one domain, and Table 26 lets a secure privileged base
 *   region carry a non-secure subregion: that last shape is the one that can
 *   serve both domains over the same bytes.
 *
 *   Candidates, programmed on the four RISAFs that see NOR traffic (the NPU's
 *   two AXI masters and the two XSPI instances), each probed through a normal
 *   secure read and an SAU-forced non-secure read of the same weight word and
 *   of one of the addresses r52 saw refused:
 *
 *     cfg0  no window at all (reset default rule)
 *     cfg1  non-secure base region           (r51)
 *     cfg2  secure base region               (r52)
 *     cfg3  secure base + non-secure subregion A
 *     cfg4  non-secure base + secure subregion A
 *
 *   The first configuration that serves both domains at both addresses wins.
 *   If none does, cfg2 is kept: the r52 run with it produced real numbers,
 *   while the r51 polarity produced nothing but zeros.
 *
 ***************************************************************************/

static uint32_t stm32n6_aton_nor_probe(uint32_t addr, uint32_t *ns)
{
  stm32n6_aton_risaf_clear();
  *ns = stm32n6_aton_ns_read32(1, addr);

  /* stm32n6_aton_ns_read32() invalidates the line it read; do the same
   * for the secure read, or this one is answered from the line the
   * previous configuration filled and every candidate looks identical.
   */

  up_invalidate_dcache((uintptr_t)addr & ~31u,
                       ((uintptr_t)addr & ~31u) + 32u);
  return getreg32(addr);
}

static void stm32n6_aton_nor_domain_apply(int cfg)
{
  static const uint32_t instances[4] =
  {
    STM32N6_ATON_RISAF4_BASE, STM32N6_ATON_RISAF5_BASE,
    STM32N6_ATON_RISAF12_BASE, STM32N6_ATON_RISAF11_BASE
  };

  static const int regions[4] = { 5, 5, 0, 0 };

  int i;

  for (i = 0; i < 4; i++)
    {
      uint32_t base   = instances[i];
      int      region = regions[i];

      stm32n6_aton_risaf_subregion(base, region, 0, 0, 0);
      stm32n6_aton_risaf_region(base, region, 0, 0, 0);

      if (cfg == 0)
        {
          continue;
        }

      stm32n6_aton_risaf_region(base, region, 0x70200000, 0x70734fff,
                                (cfg == 1 || cfg == 4)
                                  ? STM32N6_ATON_RISAF_CFGR_NS_OPEN
                                  : STM32N6_ATON_RISAF_CFGR_SEC_OPEN);

      if (cfg == 3 || cfg == 4)
        {
          stm32n6_aton_risaf_subregion(base, region, 0x70200000, 0x70734fff,
                                       STM32N6_ATON_RISAF_SR_SREN |
                                       STM32N6_ATON_RISAF_SR_RDEN |
                                       STM32N6_ATON_RISAF_SR_WREN |
                                       (cfg == 4
                                          ? STM32N6_ATON_RISAF_SR_SEC : 0));
        }
    }
}

static void stm32n6_aton_nor_domain_sweep(void)
{
  int cfg;
  int keep = 2;

  for (cfg = 0; cfg <= 4; cfg++)
    {
      uint32_t ns_a;
      uint32_t ns_b;
      uint32_t sec_a;
      uint32_t sec_b;

      stm32n6_aton_nor_domain_apply(cfg);

      sec_a = stm32n6_aton_nor_probe(0x70200000u, &ns_a);
      sec_b = stm32n6_aton_nor_probe(0x7030b3a0u, &ns_b);

      stm32n6_aton_wd_puts("[nor-sw] cfg");
      stm32n6_aton_wd_u32((uint32_t)cfg);
      stm32n6_aton_wd_puts(" a: sec=");
      stm32n6_aton_wd_hex(sec_a);
      stm32n6_aton_wd_puts(" ns=");
      stm32n6_aton_wd_hex(ns_a);
      stm32n6_aton_wd_puts(" b: sec=");
      stm32n6_aton_wd_hex(sec_b);
      stm32n6_aton_wd_puts(" ns=");
      stm32n6_aton_wd_hex(ns_b);
      stm32n6_aton_wd_puts(" latches:");
      stm32n6_aton_latch_report();
      stm32n6_aton_wd_puts("\r\n");

      if (sec_a == ns_a && ns_a != 0 && sec_b == ns_b && ns_b != 0)
        {
          keep = cfg;
        }
    }

  stm32n6_aton_nor_domain_apply(keep);

  {
    uint32_t ns_a;
    uint32_t sec_a = stm32n6_aton_nor_probe(0x70200000u, &ns_a);

    stm32n6_aton_risaf_clear();
    stm32n6_aton_wd_puts("[nor-sw] kept cfg");
    stm32n6_aton_wd_u32((uint32_t)keep);
    stm32n6_aton_wd_puts(" sec=");
    stm32n6_aton_wd_hex(sec_a);
    stm32n6_aton_wd_puts(" ns=");
    stm32n6_aton_wd_hex(ns_a);
    stm32n6_aton_wd_puts("\r\n");
  }

  stm32n6_aton_risaf_clear();
}

/****************************************************************************
 * Name: stm32n6_aton_risaf_nor_open
 *
 * Description:
 *   Let a non-secure master read the model weights, which live in the
 *   external NOR at 0x7020_0000..0x705F_FFFF.  The NPU latched exactly that
 *   denial in r31 (IAESR cid=1 ns priv read, IADDR = 0x7052_9750), and a
 *   denied read does not stall the master - it just returns zeros, so the
 *   engines were fed empty weights while the epoch still "completed".
 *
 *   Four RISAFs see that traffic:
 *
 *     RISAF4 / RISAF5  the NPU's two AXI masters: their address space is the
 *                      whole 4 GB map, so the region is programmed with
 *                      absolute addresses;
 *     RISAF12          XSPI2, the NOR: its protection space starts at
 *                      0x7000_0000, so the same bytes are addressed relative
 *                      to that base (0x0020_0000..0x005F_FFFF);
 *     RISAF11          XSPI1: left at reset, but the latches showed the NPU's
 *                      NOR reads reaching it as well, so it gets the window too.
 *
 *   The XSPI RISAFs have every region disabled at reset, so region 0 is free
 *   and it has the highest priority: nothing can shadow the new window.  On
 *   RISAF4/RISAF5 the region index 5 is the first unused one (0..4 carry the
 *   RAM windows) and it is out of the way of every other region.
 *
 *   Reads stay possible for the CPU as well: an "open" region only enables
 *   the access bits it was given, and the secure read path through such a
 *   region works (r20 proved it) - the XSPI driver keeps reading code and
 *   file data through 0x7000_0000.
 *
 ****************************************************************************/

/* --- r73 performance instrumentation ------------------------------------ */

static bool     g_aton_perf_diag = true;  /* per-run diagnostics (run 1 only) */
static unsigned g_aton_perf_runs;         /* inferences since boot            */
static uint32_t g_aton_perf_c0;           /* DWT stamp: before the prep       */
static uint32_t g_aton_perf_c1;           /* ... before the NPU run           */
static uint32_t g_aton_perf_c2;           /* ... after the NPU run            */
static uint32_t g_aton_perf_c3;

/* The DWT cycle counter is enabled by the ATON OSAL (DEMCR.TRCENA plus
 * DWT_CTRL.CYCCNTENA); reading it costs a couple of cycles and gives the
 * CPU-side phases a resolution the 1 ms system timer cannot offer.
 */

#define STM32N6_ATON_DWT_CYCCNT 0xe0001004u

static uint32_t stm32n6_aton_cyccnt(void)
{
  return getreg32(STM32N6_ATON_DWT_CYCCNT);
}

/****************************************************************************
 * Name: stm32n6_aton_weights_check
 *
 * Description:
 *   Check that the model weights are still present in the external NOR.
 *
 *   The model weights live in the external NOR and are programmed separately
 *   from files that carry their own addresses: the eye model's initializers at
 *   0x70400000 (eye_flash/eye-data.hex, 2,899,745 B) and the face model's at
 *   0x70700000 (face_flash/face-data.hex, 109,105 B).  The NuttX image itself
 *   is loaded from 0x70100000 and must stay under 1 MiB, because the FSBL
 *   copies only that much into SRAM before jumping.
 *
 *   A flash run that overruns its window does not fail loudly: it silently
 *   overwrites the weights instead.  The NPU keeps running afterwards -
 *   deterministically, without an error and with the epoch events still
 *   arriving - so every later test measures garbage, the fill fingerprints
 *   shift and all detections come out negative.
 *
 *   That is what happened to the images flashed between r63 and r68: the four
 *   rounds spent on the compiler (-O0/-Os), on the preprocessing and on the
 *   tensor pool were all chasing a NOR that no longer held the model.  Hence
 *   this check - one line at boot says whether the weights are there.
 *
 ****************************************************************************/

/* The first six words are spread over the eye model's initializers
 * (0x70400000..0x706c3f21, 2,899,745 B, flashed from eye_flash/eye-data.hex);
 * the last five over the face model's (0x70700000..0x7071aa30, 109,105 B,
 * flashed from network_atonbuf.xSPI2.raw).  Neither range is all-zero or
 * erased, so a mismatch means the flash image is stale.
 */

static const uint32_t g_aton_weights_addr[] =
{
  0x70400000u, 0x70475f7cu, 0x704ebefcu,
  0x70561e7cu, 0x705d7dfcu, 0x7064dd7cu,
  0x70700000u, 0x70705534u, 0x7070aa6cu,
  0x7070ffa4u, 0x70715690u,
};

static const uint32_t g_aton_weights_want[] =
{
  /* v3 (five-class) fingerprint, read from eye_flash/eye_model_data.xSPI2.bin.
   * The previous values were the two-class model, so every boot reported the
   * weights as corrupted even though the flash image was byte-identical to the
   * file that had been flashed.  A stale fingerprint is worse than none: it
   * buries real faults under a permanent warning.
   */

  0xc2fc16fcu, 0xe9c11427u, 0xe3f70101u,
  0x75af3006u, 0xf2d011b9u, 0xd792203bu,
  0x000000e5u, 0x00fa0000u, 0x00005700u,
  0x1034f456u, 0x1636be44u,
};

#define STM32N6_ATON_WEIGHTS_NITEMS \
  (sizeof(g_aton_weights_addr) / sizeof(g_aton_weights_addr[0]))

static void stm32n6_aton_weights_check(void)
{
  unsigned int i;
  uint32_t ok = 0;

  for (i = 0; i < STM32N6_ATON_WEIGHTS_NITEMS; i++)
    {
      uint32_t got = getreg32(g_aton_weights_addr[i]);

      if (got == g_aton_weights_want[i])
        {
          ok++;
          continue;
        }

      stm32n6_aton_wd_puts("[aton] weights @");
      stm32n6_aton_wd_hex(g_aton_weights_addr[i]);
      stm32n6_aton_wd_puts(" = ");
      stm32n6_aton_wd_hex(got);
      stm32n6_aton_wd_puts(", want ");
      stm32n6_aton_wd_hex(g_aton_weights_want[i]);
      stm32n6_aton_wd_puts(" <<< MISMATCH\r\n");
    }

  stm32n6_aton_wd_puts("[aton] weights check: ");
  stm32n6_aton_wd_u32(ok);
  stm32n6_aton_wd_puts("/");
  stm32n6_aton_wd_u32((uint32_t)STM32N6_ATON_WEIGHTS_NITEMS);
  stm32n6_aton_wd_puts(" sampled words match - ");

  if (ok == STM32N6_ATON_WEIGHTS_NITEMS)
    {
      stm32n6_aton_wd_puts("the eye and face weights are in NOR\r\n");
    }
  else
    {
      stm32n6_aton_wd_puts("*** WEIGHTS CORRUPTED: re-flash "
                           "eye-data.hex at 0x70400000 and "
                           "face-data.hex at 0x70700000, and keep every "
                           "image <= 1 MiB ***\r\n");
    }

  /* Those reads go through the freshly reprogrammed window, so drop whatever
   * latch they produced: a stale one would look like a fresh NPU fault in a
   * later stall dump.
   */

  stm32n6_aton_risaf_clear();
}

static void stm32n6_aton_risaf_nor_open(void)
{
  /* The CPU still sees the weights until the non-secure region is in the way:
   * read them first, so one boot log shows both the "normal" values and what
   * the window does to a secure reader afterwards.
   */

  stm32n6_aton_wd_puts("[nor] cpu pre : w@eye=");
  stm32n6_aton_wd_hex(getreg32(0x70400000u));
  stm32n6_aton_wd_puts(" w@face=");
  stm32n6_aton_wd_hex(getreg32(0x70700000u));
  stm32n6_aton_wd_puts("\r\n");

  /* r52: the window serves the SECURE domain.  The r51 log latched
   * `risaf11 addr=703246e8 cid=1 sec priv read` during the model run: a
   * privileged secure read of a weight address was being refused because this
   * very region was opened for non-secure masters only - and a region serves
   * exactly one security domain.  A refused read neither stalls nor errors, it
   * returns zeros, so the engines were fed empty weights while the run still
   * reported success.  That is the all-+/-0.0 output of both models.
   *
   * Serving the secure domain matches the reset rule the vendor example relies
   * on (secure + privileged + CID1 allowed, nothing else programmed).  The
   * non-secure probe at the end of this function now expects zeros: the only
   * non-secure NOR reader we ever had was that probe itself.
   */

  /* r54: the NOR has TWO kinds of reader and one RISAF region serves a single
   * security domain:
   *
   *   the NPU's stream engines      non-secure   (r52/r53 latches: iaesr=0x11)
   *   the CPU-side software epochs  secure       (r51 showed their reads
   *                                              denied => output all +/-0.0)
   *
   * Splitting the range would need knowing which weights each side touches, so
   * instead the CPU's own view of the weight window is switched to the
   * non-secure domain - an SAU region, the same mechanism the probe helper
   * uses.  One non-secure window then covers every reader.
   *
   * The SAU must be disabled while its regions change, and region 0 (the SRAM
   * alias) has to be re-programmed in the same pass because the array is only
   * writable while the unit is off.
   */

  putreg32(0, STM32N6_ATON_SAU_CTRL);
  stm32n6_aton_sau_ns_region(0, STM32N6_ATON_NS_ALIAS_BASE,
                             STM32N6_ATON_NS_ALIAS_SIZE);
  stm32n6_aton_sau_ns_region(2, 0x70200000, 0x00540000);
  putreg32(0x1, STM32N6_ATON_SAU_CTRL);
  stm32n6_aton_wd_puts("[nor] sau: alias + weight window are non-secure\r\n");

  /* The sweep is kept for the record, but the winner is chosen by policy now:
   * with the SAU putting this window on the non-secure side for the CPU as
   * well, the non-secure configuration (cfg1) is the only one that serves BOTH
   * readers.  r54 let the sweep decide and it kept the secure window, which
   * sent the CPU-side readers back to zeros (checksum 0x00000000) - the same
   * failure mode r51 had, just from the other direction.
   */

  stm32n6_aton_nor_domain_sweep();
  stm32n6_aton_nor_domain_apply(1);
  stm32n6_aton_risaf_clear();
  stm32n6_aton_wd_puts("[nor] window forced to cfg1 (non-secure; serves the "
                       "SAU non-secure CPU and the NPU)\r\n");

  stm32n6_aton_wd_puts("[nor] window regs: risaf4 r6=");
  stm32n6_aton_wd_hex(getreg32(STM32N6_ATON_RISAF4_BASE + STM32N6_ATON_RISAF_REG0 +
                               5 * STM32N6_ATON_RISAF_REGION_STRIDE));
  stm32n6_aton_wd_puts(" risaf5 r6=");
  stm32n6_aton_wd_hex(getreg32(STM32N6_ATON_RISAF5_BASE + STM32N6_ATON_RISAF_REG0 +
                               5 * STM32N6_ATON_RISAF_REGION_STRIDE));
  stm32n6_aton_wd_puts(" risaf12 r1=");
  stm32n6_aton_wd_hex(getreg32(STM32N6_ATON_RISAF12_BASE + STM32N6_ATON_RISAF_REG0));
  stm32n6_aton_wd_puts(" risaf11 r1=");
  stm32n6_aton_wd_hex(getreg32(STM32N6_ATON_RISAF11_BASE + STM32N6_ATON_RISAF_REG0));
  stm32n6_aton_wd_puts("\r\n");

  /* The secure read the CPU performs here is the same kind of access the NPU's
   * weight port issues, so it must survive the window: the two words printed
   * below are the pass/fail line for r52.  They have to match `[nor] cpu pre`
   * (0xdbfd26e4 / 0x000000e5 for the eye / face models).
   */

  stm32n6_aton_wd_puts("[nor] cpu post: w@eye=");
  stm32n6_aton_wd_hex(getreg32(0x70400000u));
  stm32n6_aton_wd_puts(" w@face=");
  stm32n6_aton_wd_hex(getreg32(0x70700000u));
  stm32n6_aton_wd_puts(" (== cpu pre => the kept config serves the secure "
                      "domain)\r\n");

  /* That read was a deliberate secure access through a non-secure window, so it
   * latched RISAF11/RISAF12 as an illegal access.  Drop the latches again: a
   * stale one would look like a fresh NPU fault in any later stall dump, which
   * is exactly the kind of false lead that costs a whole round.
   */

  stm32n6_aton_risaf_clear();
  stm32n6_aton_wd_puts("[nor] latches cleared after the probe\r\n");

  /* Mirror image of the line above, and intentionally the failing direction
   * now: the window serves the secure domain, so a non-secure reader gets
   * zeros plus a latch.  Kept as evidence that the policy really changed.
   */

  stm32n6_aton_risaf_clear();
  stm32n6_aton_wd_puts("[nor-ns] ns read w@eye=");
  stm32n6_aton_wd_hex(stm32n6_aton_ns_read32(1, 0x70400000u));
  stm32n6_aton_wd_puts(" latches:");
  stm32n6_aton_latch_report();
  stm32n6_aton_risaf_clear();

  /* With the window in place, check that the model is still in the NOR: this
   * is the one failure mode that leaves every other sub-system green (no
   * busfault, epoch events arriving, plausible-looking logits) while all
   * detections are wrong.
   */

  stm32n6_aton_weights_check();
}

/****************************************************************************
 * Name: stm32n6_aton_risaf_map_dump
 *
 * Description:
 *   Print the protection map of RISAF1..RISAF9 (region 0 attributes plus the
 *   pending illegal-access flags) so a single boot log shows which address
 *   spaces are open to the NPU and which are not.
 *
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_aton_risaf_xspi_dump
 *
 * Description:
 *   Dump the three RISAFs guarding the external memory windows (RISAF11 =
 *   XSPI1, RISAF12 = XSPI2/NOR, RISAF13 = XSPI3): their first four base
 *   regions plus the most recent latched illegal access.
 *
 *   A read that RISAF rejects returns zeros and latches IASR, and IAESR bit 5
 *   then says whether the rejected request was secure or non-secure.  If the
 *   NPU latched it, this is the direct answer to "may the NPU read the
 *   weights, and with which attributes".
 *
 ****************************************************************************/

static void stm32n6_aton_risaf_xspi_dump(void)
{
  static const uint32_t xspi[STM32N6_ATON_NXSPI] =
  {
    STM32N6_ATON_RISAF11_BASE,
    STM32N6_ATON_RISAF12_BASE,
    STM32N6_ATON_RISAF13_BASE
  };

  int i;
  int r;

  for (i = 0; i < STM32N6_ATON_NXSPI; i++)
    {
      uint32_t iasr  = getreg32(xspi[i] + STM32N6_ATON_RISAF_IASR);
      uint32_t iaesr = getreg32(xspi[i] + STM32N6_ATON_RISAF_IAESR);
      uint32_t iaddr = getreg32(xspi[i] + STM32N6_ATON_RISAF_IADDR);

      stm32n6_aton_wd_puts("[xspi] risaf");
      stm32n6_aton_wd_u32(11 + i);
      stm32n6_aton_wd_puts(" cr=");
      stm32n6_aton_wd_hex(getreg32(xspi[i] + STM32N6_ATON_RISAF_CR));
      stm32n6_aton_wd_puts(" iasr=");
      stm32n6_aton_wd_hex(iasr);
      stm32n6_aton_wd_puts(" iaesr=");
      stm32n6_aton_wd_hex(iaesr);
      stm32n6_aton_wd_puts(" iaddr=");
      stm32n6_aton_wd_hex(iaddr);

      if ((iasr & 0x2) != 0)
        {
          stm32n6_aton_wd_puts(" ILLEGAL cid=");
          stm32n6_aton_wd_u32(iaesr & 0x7);
          stm32n6_aton_wd_puts((iaesr & 0x20) ? " sec" : " ns");
          stm32n6_aton_wd_puts((iaesr & 0x10) ? " priv" : " unpriv");
          stm32n6_aton_wd_puts((iaesr & 0x80) ? " write" : " read");
        }

      stm32n6_aton_wd_puts("\r\n");

      for (r = 0; r < 4; r++)
        {
          uint32_t reg = xspi[i] + STM32N6_ATON_RISAF_REG0 +
                         (uint32_t)r * STM32N6_ATON_RISAF_REGION_STRIDE;

          stm32n6_aton_wd_puts("[xspi] risaf");
          stm32n6_aton_wd_u32(11 + i);
          stm32n6_aton_wd_puts(" r");
          stm32n6_aton_wd_u32(r + 1);
          stm32n6_aton_wd_puts(" cfgr=");
          stm32n6_aton_wd_hex(getreg32(reg + 0x00));
          stm32n6_aton_wd_puts(" start=");
          stm32n6_aton_wd_hex(getreg32(reg + 0x04));
          stm32n6_aton_wd_puts(" end=");
          stm32n6_aton_wd_hex(getreg32(reg + 0x08));
          stm32n6_aton_wd_puts(" cidc=");
          stm32n6_aton_wd_hex(getreg32(reg + 0x0c));
          stm32n6_aton_wd_puts("\r\n");
        }
    }
}

static void stm32n6_aton_risaf_map_dump(const char *tag)
{
  static const uint32_t risaf[STM32N6_ATON_NRISAF] =
  {
    STM32N6_ATON_RISAF1_BASE, STM32N6_ATON_RISAF2_BASE,
    STM32N6_ATON_RISAF3_BASE, STM32N6_ATON_RISAF4_BASE,
    STM32N6_ATON_RISAF5_BASE, STM32N6_ATON_RISAF6_BASE,
    STM32N6_ATON_RISAF7_BASE, STM32N6_ATON_RISAF8_BASE,
    STM32N6_ATON_RISAF9_BASE, STM32N6_ATON_RISAF11_BASE,
    STM32N6_ATON_RISAF12_BASE, STM32N6_ATON_RISAF13_BASE
  };

  int i;

  for (i = 0; i < STM32N6_ATON_NRISAF; i++)
    {
      uint32_t reg0 = risaf[i] + STM32N6_ATON_RISAF_REG0;

      /* Raw up_putc output on purpose: this has to survive a wedged console
       * and CONFIG_DEBUG_INFO is off in the NPU configuration, so _info()
       * would print nothing at all.
       */

      stm32n6_aton_wd_puts("[risaf-");
      stm32n6_aton_wd_puts(tag);
      stm32n6_aton_wd_puts("] risaf");
      stm32n6_aton_wd_u32(i + 1);
      stm32n6_aton_wd_puts(" cfgr=");
      stm32n6_aton_wd_hex(getreg32(reg0 + 0x00));
      stm32n6_aton_wd_puts(" start=");
      stm32n6_aton_wd_hex(getreg32(reg0 + 0x04));
      stm32n6_aton_wd_puts(" end=");
      stm32n6_aton_wd_hex(getreg32(reg0 + 0x08));
      stm32n6_aton_wd_puts(" cidc=");
      stm32n6_aton_wd_hex(getreg32(reg0 + 0x0c));
      stm32n6_aton_wd_puts(" iasr=");
      stm32n6_aton_wd_hex(getreg32(risaf[i] + STM32N6_ATON_RISAF_IASR));

      /* Region 1 is the non-secure window on the instances that carry one:
       * print it so a single boot log shows whether it really took.
       */

      if (getreg32(reg0 + STM32N6_ATON_RISAF_REGION_STRIDE) != 0)
        {
          stm32n6_aton_wd_puts(" | r1 cfgr=");
          stm32n6_aton_wd_hex(getreg32(reg0 +
                            STM32N6_ATON_RISAF_REGION_STRIDE));
          stm32n6_aton_wd_puts(" start=");
          stm32n6_aton_wd_hex(getreg32(reg0 +
                            STM32N6_ATON_RISAF_REGION_STRIDE + 0x04));
          stm32n6_aton_wd_puts(" end=");
          stm32n6_aton_wd_hex(getreg32(reg0 +
                            STM32N6_ATON_RISAF_REGION_STRIDE + 0x08));
          stm32n6_aton_wd_puts(" cidc=");
          stm32n6_aton_wd_hex(getreg32(reg0 +
                            STM32N6_ATON_RISAF_REGION_STRIDE + 0x0c));
        }

      stm32n6_aton_wd_puts("\r\n");
    }
}

/****************************************************************************
 * Name: stm32n6_aton_risaf_clear
 *
 * Description:
 *   Clear any stale illegal-access records so that the next fault dump shows
 *   only what actually happened in this boot.
 *
 ****************************************************************************/

static void stm32n6_aton_risaf_clear(void)
{
  static const uint32_t risaf[STM32N6_ATON_NRISAF] =
  {
    STM32N6_ATON_RISAF1_BASE, STM32N6_ATON_RISAF2_BASE,
    STM32N6_ATON_RISAF3_BASE, STM32N6_ATON_RISAF4_BASE,
    STM32N6_ATON_RISAF5_BASE, STM32N6_ATON_RISAF6_BASE,
    STM32N6_ATON_RISAF7_BASE, STM32N6_ATON_RISAF8_BASE,
    STM32N6_ATON_RISAF9_BASE, STM32N6_ATON_RISAF11_BASE,
    STM32N6_ATON_RISAF12_BASE, STM32N6_ATON_RISAF13_BASE
  };

  int i;

  for (i = 0; i < STM32N6_ATON_NRISAF; i++)
    {
      uint32_t iasr = getreg32(risaf[i] + STM32N6_ATON_RISAF_IASR);

      if (iasr != 0)
        {
          putreg32(iasr, risaf[i] + STM32N6_ATON_RISAF_IACR);
        }
    }
}

/****************************************************************************
 * Name: stm32n6_aton_watchdog
 *
 * Description:
 *   Give an in-flight inference a voice.
 *
 *   When the NPU keeps its interrupt line asserted, the CPU never returns to
 *   thread mode: the ISR completes and the NVIC takes the same interrupt
 *   again, so nothing can report and the shell just stops with no message.
 *   This thread reports the last runtime phase and the interrupt/event
 *   counters once a second, and dumps the runtime's ISR-side log as soon as
 *   the storm protection had to mask the line.
 *
 ****************************************************************************/

static void stm32n6_aton_wd_puts(FAR const char *str)
{
  while (*str != '\0')
    {
      up_putc((int)(unsigned char)*str++);
    }
}

static void stm32n6_aton_wd_u32(uint32_t value)
{
  char buf[11];
  int  n = 0;

  if (value == 0)
    {
      up_putc('0');
      return;
    }

  while (value != 0 && n < (int)sizeof(buf))
    {
      buf[n++] = (char)('0' + (value % 10));
      value   /= 10;
    }

  while (n-- > 0)
    {
      up_putc(buf[n]);
    }
}

static void stm32n6_aton_wd_hex(uint32_t value)
{
  int i;

  for (i = 28; i >= 0; i -= 4)
    {
      uint32_t nib = (value >> i) & 0xf;
      up_putc((int)(nib < 10 ? '0' + nib : 'a' + nib - 10));
    }
}

static void stm32n6_aton_wd_task(FAR struct tcb_s *tcb, FAR void *arg)
{
  UNUSED(arg);

  stm32n6_aton_wd_puts(" ");
  stm32n6_aton_wd_puts(tcb->name);
  stm32n6_aton_wd_puts(":");
  stm32n6_aton_wd_u32((uint32_t)tcb->task_state);
  stm32n6_aton_wd_puts("@");
  stm32n6_aton_wd_hex((uint32_t)(uintptr_t)tcb->waitobj);
}

static uint32_t stm32n6_aton_wd_primask(void)
{
  uint32_t value;
  __asm__ __volatile__("mrs %0, primask" : "=r"(value));
  return value;
}

static uint32_t stm32n6_aton_wd_basepri(void)
{
  uint32_t value;
  __asm__ __volatile__("mrs %0, basepri" : "=r"(value));
  return value;
}

/* Heartbeat.
 *
 * Written with polled, unbuffered up_putc() on purpose: this line has to get
 * out even when the console's interrupt driven path, its lock or the scheduler
 * are in trouble - that is exactly the situation it exists to diagnose.
 *
 * Reading:
 *   - heartbeat continues after the app went quiet -> the CPU and the tick are
 *     alive, the application (or the console path) is what stopped;
 *   - heartbeat stops -> the CPU never returns to thread mode (interrupt
 *     storm, interrupt handler stuck) or interrupts are masked - primask /
 *     basepri show which;
 *   - tick not advancing while the heartbeat runs -> the system tick is gone.
 */

/****************************************************************************
 * Name: stm32n6_aton_npu_bus_dump
 *
 * Description:
 *   Dump the NPU's bus interfaces and its AXI cache.  BUSIF0/1 latch one
 *   error bit per AXI bus port ("Errors from AXI bus related to busport N"),
 *   which is where a read that no slave answers or that the interconnect
 *   rejects shows up - the epoch controller reports neither as an interrupt.
 *   The cache status says whether the cache is idle, busy or errored, and
 *   which address range the last maintenance command covered.
 *
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_aton_npu_clocks_on
 *
 * Description:
 *   Open every ATON clock gate (AGATES0/1 and BGATES).  The vendor runtime
 *   closes them when it is built with LL_ATON_ENABLE_CLOCK_GATING=1; our OSAL
 *   hook already reopens them at runtime init, and this is the belt-and-
 *   braces copy the watchdog repeats if the epoch loop still stalls.
 *
 ****************************************************************************/

/* Keep the NPU domain clocked while the CPU sleeps.
 *
 * This port fills in only the low-power (CSLEEP) clock enables it needs for
 * itself - bus, memories, USART1 - while RCC_AHB5LPENR resets to zero.  The
 * NPU and CACHEAXI clocks therefore stop the moment up_idle() executes WFI,
 * which is what happens a few microseconds after the epoch controller starts
 * the dataflow: the stream engines wedge mid transfer and the epoch
 * controller waits for a frame that can never finish.  The ST examples avoid
 * this by enabling every low-power clock in low_power_clock_config().
 */

static void stm32n6_aton_gate_dump(void)
{
  stm32n6_aton_wd_puts("[gate] scr=");
  stm32n6_aton_wd_hex(getreg32(0xe000ed10u));
  stm32n6_aton_wd_puts(" mscr=");
  stm32n6_aton_wd_hex(getreg32(0xe001e000u));
  stm32n6_aton_wd_puts(" icncgcr=");
  stm32n6_aton_wd_hex(getreg32(STM32_SYSCFG_BASE + 0x38));
  stm32n6_aton_wd_puts(" npuicncr=");
  stm32n6_aton_wd_hex(getreg32(STM32_SYSCFG_BASE + 0x78));
  stm32n6_aton_wd_puts(" busenr=");
  stm32n6_aton_wd_hex(getreg32(STM32_RCC_BASE + 0x244));
  stm32n6_aton_wd_puts(" miscenr=");
  stm32n6_aton_wd_hex(getreg32(STM32_RCC_BASE + 0x248));
  stm32n6_aton_wd_puts(" memenr=");
  stm32n6_aton_wd_hex(getreg32(STM32_RCC_BASE + 0x24c));
  stm32n6_aton_wd_puts(" buslpen=");
  stm32n6_aton_wd_hex(getreg32(STM32_RCC_BASE + 0x284));
  stm32n6_aton_wd_puts(" ahb5lpen=");
  stm32n6_aton_wd_hex(getreg32(STM32_RCC_BASE + 0x2a0));
  stm32n6_aton_wd_puts("\r\n");
}

static void stm32n6_aton_enable_sleep_clocks(void)
{
  static const uint32_t lpen[15] =
  {
    STM32_RCC_BASE + 0x0284,  /* BUSLPENR: ACLKN(NPU ICN) */
    STM32_RCC_BASE + 0x0288,  /* MISCLPENR (ST sets ~0 here too) */
    STM32_RCC_BASE + 0x028c,  /* MEMLPENR   */
    STM32_RCC_BASE + 0x0290,  /* AHB1LPENR  */
    STM32_RCC_BASE + 0x0294,  /* AHB2LPENR  */
    STM32_RCC_BASE + 0x0298,  /* AHB3LPENR: RISAF/RIFSC/IAC */
    STM32_RCC_BASE + 0x029c,  /* AHB4LPENR  */
    STM32_RCC_BASE + 0x02a0,  /* AHB5LPENR: NPU, CACHEAXI   */
    STM32_RCC_BASE + 0x02a4,  /* APB1LPENR1 */
    STM32_RCC_BASE + 0x02a8,  /* APB1LPENR2 */
    STM32_RCC_BASE + 0x02ac,  /* APB2LPENR  */
    STM32_RCC_BASE + 0x02b0,  /* APB3LPENR  */
    STM32_RCC_BASE + 0x02b4,  /* APB4LPENR1 */
    STM32_RCC_BASE + 0x02b8,  /* APB4LPENR2 */
    STM32_RCC_BASE + 0x02bc   /* APB5LPENR  */
  };

  int i;

  /* 15 项：BUS/MISC/MEM + AHB1..5 + APB1..5。
   * 这里曾经写成 lpen[14] 且循环 14 次，导致初始izer里第 15 个
   * APB5LPENR 被静默丢弃 —— APB5 承载 LTDC 的时钟门控。 */

  for (i = 0; i < 15; i++)
    {
      putreg32(getreg32(lpen[i]) | 0xffffffffu, lpen[i]);
    }

  stm32n6_aton_wd_puts("[sleep] bus=");
  stm32n6_aton_wd_hex(getreg32(STM32_RCC_BASE + 0x0284));
  stm32n6_aton_wd_puts(" mem=");
  stm32n6_aton_wd_hex(getreg32(STM32_RCC_BASE + 0x028c));
  stm32n6_aton_wd_puts(" ahb3=");
  stm32n6_aton_wd_hex(getreg32(STM32_RCC_BASE + 0x0298));
  stm32n6_aton_wd_puts(" ahb5=");
  stm32n6_aton_wd_hex(getreg32(STM32_RCC_BASE + 0x02a0));
  stm32n6_aton_wd_puts("\\r\\n");
}

static void stm32n6_aton_npu_clocks_on(void)
{
  putreg32(1, STM32_ATON_BLK_CLKCTRL);
  putreg32(0xffffffff, STM32_ATON_CLKCTRL_AGATES0);
  putreg32(0xffffffff, STM32_ATON_CLKCTRL_AGATES1);
  putreg32(0xffffffff, STM32_ATON_CLKCTRL_BGATES);

  stm32n6_aton_wd_puts("[npu] clocks opened: ag0=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_CLKCTRL_AGATES0));
  stm32n6_aton_wd_puts(" ag1=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_CLKCTRL_AGATES1));
  stm32n6_aton_wd_puts(" bg=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_CLKCTRL_BGATES));
  stm32n6_aton_wd_puts("\r\n");
  /* Baseline of the central stream switch, taken while the model is being
   * brought up (before any epoch runs): later dumps can then tell "never
   * configured" apart from "configured and then cleared".
   */

  stm32n6_aton_wd_puts("[switch] base ctrl=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRSWITCH_CTRL));
  stm32n6_aton_wd_puts(" ver=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRSWITCH_VERSION));
  stm32n6_aton_wd_puts(" dst0..7:");
  for (int si = 0; si < 8; si++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRSWITCH_DST(si)));
    }

  stm32n6_aton_wd_puts("\r\n");


  /* The NPU domain must stay clocked while the CPU idles (see above) */

  stm32n6_aton_enable_sleep_clocks();

  /* r31: MEMSYSCTL MSCR - mark both CPU caches active, exactly like the ST
   * 994 example's main() (MSCR |= ICACTIVE | DCACTIVE); the memory system
   * uses those bits to know the caches are live.
   */

  putreg32(getreg32(0xe001e000u) | (1u << 13) | (1u << 12), 0xe001e000u);
  stm32n6_aton_gate_dump();

}

/****************************************************************************
 * Name: stm32n6_aton_npu_block_dump
 *
 * Description:
 *   Print the CTRL register of every ATON unit plus the clock control unit's
 *   gates.  Each unit's CTRL bit 0 is its enable; a unit that is disabled (or
 *   clock gated) stalls the epoch controller on the opcode that needs it, and
 *   the epoch controller reports neither an error interrupt nor a counter
 *   change for that - which is exactly the shape of the current hang.
 *
 ****************************************************************************/

static void stm32n6_aton_npu_block_dump(void)
{
  static const uint32_t blk[12] =
  {
    STM32_ATON_BLK_CLKCTRL, STM32_ATON_BLK_INTCTRL,
    STM32_ATON_BLK_BUSIF0, STM32_ATON_BLK_BUSIF1,
    STM32_ATON_BLK_STRENG, STM32_ATON_BLK_CONVACC,
    STM32_ATON_BLK_DECUN, STM32_ATON_BLK_ACTIV,
    STM32_ATON_BLK_ARITH, STM32_ATON_BLK_POOL,
    STM32_ATON_BLK_RECBUF, STM32_ATON_BLK_EPOCHCTRL
  };

  static const char * const name[12] =
  {
    "clk", "int", "busif0", "busif1", "streng",
    "convacc", "decun", "activ", "arith", "pool", "recbuf", "ec"
  };

  int i;

  stm32n6_aton_wd_puts("[npu] ctrl");

  for (i = 0; i < 12; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_puts(name[i]);
      stm32n6_aton_wd_puts("=");
      stm32n6_aton_wd_hex(getreg32(blk[i]));
    }

  stm32n6_aton_wd_puts("\r\n");

  stm32n6_aton_wd_puts("[npu] gates ag0=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_CLKCTRL_AGATES0));
  stm32n6_aton_wd_puts(" ag1=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_CLKCTRL_AGATES1));
  stm32n6_aton_wd_puts(" bg=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_CLKCTRL_BGATES));
  stm32n6_aton_wd_puts("\r\n");
}

static void stm32n6_aton_npu_bus_dump(void)
{
  stm32n6_aton_wd_puts("[npu] xspi2 cr=");
  stm32n6_aton_wd_hex(getreg32(0x4802a000));
  stm32n6_aton_wd_puts(" sr=");
  stm32n6_aton_wd_hex(getreg32(0x4802a020));
  stm32n6_aton_wd_puts(" ccr=");
  stm32n6_aton_wd_hex(getreg32(0x4802a100));
  stm32n6_aton_wd_puts(" tcr=");
  stm32n6_aton_wd_hex(getreg32(0x4802a108));
  stm32n6_aton_wd_puts("\r\n");

  stm32n6_aton_wd_puts("[npu] busif0 ctrl=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_BUSIF0_CTRL));
  stm32n6_aton_wd_puts(" err=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_BUSIF0_ERR));
  stm32n6_aton_wd_puts(" busif1 ctrl=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_BUSIF1_CTRL));
  stm32n6_aton_wd_puts(" err=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_BUSIF1_ERR));
  stm32n6_aton_wd_puts("\r\n");

  stm32n6_aton_wd_puts("[npu] cache sr=");
  stm32n6_aton_wd_hex(getreg32(STM32_CACHEAXI_SR));
  stm32n6_aton_wd_puts(" cr1=");
  stm32n6_aton_wd_hex(getreg32(STM32_CACHEAXI_CR1));
  stm32n6_aton_wd_puts(" cr2=");
  stm32n6_aton_wd_hex(getreg32(STM32_CACHEAXI_CR2));
  stm32n6_aton_wd_puts(" cmdstart=");
  stm32n6_aton_wd_hex(getreg32(STM32_CACHEAXI_CMDRSADDRR));
  stm32n6_aton_wd_puts(" cmdend=");
  stm32n6_aton_wd_hex(getreg32(STM32_CACHEAXI_CMDREADDRR));
  stm32n6_aton_wd_puts("\r\n");
}

/****************************************************************************
 * Name: stm32n6_aton_ns_read32
 *
 * Description:
 *   Read one word the way a non-secure master would read it, by temporarily
 *   attributing the address to an SAU region.  The SAU may only be modified
 *   while disabled, so: disable, program region, enable, read, disable,
 *   release region, enable.  Region 0 (the blob window) is left alone.
 *
 ****************************************************************************/

static uint32_t stm32n6_aton_ns_read32(int region, uint32_t addr)
{
  irqstate_t flags;
  uint32_t   value;

  flags = up_irq_save();

  putreg32(0, STM32N6_ATON_SAU_CTRL);
  stm32n6_aton_sau_ns_region(region, addr, 32);
  putreg32(1, STM32N6_ATON_SAU_CTRL);

  /* The SAU only changes the security attribute the core presents on
   * the bus; it does not change where the load is served from.  With
   * the D-Cache enabled, the line the preceding secure read filled
   * would satisfy this read as well: the access never reaches the RIF,
   * and a refused configuration becomes indistinguishable from an
   * accepted one (every candidate reports the same value, so
   * nor_domain_sweep() keeps whichever it tried last).  Drop the line
   * first so the read really reaches the NOR, and drop it again
   * afterwards so a rejected value cannot stay behind and poison the
   * weight check.
   */

  up_invalidate_dcache((uintptr_t)addr & ~31u,
                       ((uintptr_t)addr & ~31u) + 32u);
  value = getreg32(addr);
  up_invalidate_dcache((uintptr_t)addr & ~31u,
                       ((uintptr_t)addr & ~31u) + 32u);

  putreg32(0, STM32N6_ATON_SAU_CTRL);
  putreg32((uint32_t)region, STM32N6_ATON_SAU_RNR);
  putreg32(0, STM32N6_ATON_SAU_RBAR);
  putreg32(0, STM32N6_ATON_SAU_RLAR);
  putreg32(1, STM32N6_ATON_SAU_CTRL);

  up_irq_restore(flags);
  return value;
}

/****************************************************************************
 * Name: stm32n6_aton_latch_report
 *
 * Description:
 *   Print which RISAF instances have an illegal access latched right now.
 *
 ****************************************************************************/

static void stm32n6_aton_latch_report(void)
{
  static const uint32_t risaf[STM32N6_ATON_NRISAF] =
  {
    STM32N6_ATON_RISAF1_BASE, STM32N6_ATON_RISAF2_BASE,
    STM32N6_ATON_RISAF3_BASE, STM32N6_ATON_RISAF4_BASE,
    STM32N6_ATON_RISAF5_BASE, STM32N6_ATON_RISAF6_BASE,
    STM32N6_ATON_RISAF7_BASE, STM32N6_ATON_RISAF8_BASE,
    STM32N6_ATON_RISAF9_BASE, STM32N6_ATON_RISAF11_BASE,
    STM32N6_ATON_RISAF12_BASE, STM32N6_ATON_RISAF13_BASE
  };

  int i;

  stm32n6_aton_wd_puts("[latch]");

  for (i = 0; i < STM32N6_ATON_NRISAF; i++)
    {
      uint32_t iasr = getreg32(risaf[i] + STM32N6_ATON_RISAF_IASR);

      if (iasr != 0)
        {
          uint32_t iaesr = getreg32(risaf[i] + STM32N6_ATON_RISAF_IAESR);
          uint32_t iaddr = getreg32(risaf[i] + STM32N6_ATON_RISAF_IADDR);

          stm32n6_aton_wd_puts(" risaf");
          stm32n6_aton_wd_u32(i < 9 ? i + 1 : 11 + (i - 9));
          stm32n6_aton_wd_puts("=");
          stm32n6_aton_wd_hex(iasr);
          stm32n6_aton_wd_puts("/");
          stm32n6_aton_wd_hex(iaddr);
          stm32n6_aton_wd_puts("/cid");
          stm32n6_aton_wd_u32(iaesr & 0x7);
          stm32n6_aton_wd_puts((iaesr & 0x20) ? "sec" : "ns");
        }
    }

  stm32n6_aton_wd_puts("\r\n");
}

/****************************************************************************
 * Name: stm32n6_aton_matrix_probe
 *
 * Description:
 *   Decide, on hardware, how RISAF answers a read it rejects: silently with
 *   zeros, or with a latched illegal access.  Both the secure and the
 *   non-secure view of the same word are read, and the latches are printed
 *   after each one, because the whole "the NPU was not denied" conclusion
 *   rests on which of the two it is.
 *
 ****************************************************************************/

static void stm32n6_aton_matrix_probe(void)
{
  static const uint32_t addrs[4] =
  {
    0x70200000UL,   /* NOR window     (RISAF12) */
    0x24200000UL,   /* NPU RAM pool   (RISAF4/5/6) */
    0x24300000UL,   /* blob window    (RISAF1/4/5/6) */
    0x243c0000UL    /* NPU cache RAM  (RISAF8) */
  };

  int i;

  for (i = 0; i < 4; i++)
    {
      stm32n6_aton_risaf_clear();
      stm32n6_aton_wd_puts("[probe] addr=0x");
      stm32n6_aton_wd_hex(addrs[i]);
      stm32n6_aton_wd_puts(" sec=");
      stm32n6_aton_wd_hex(getreg32(addrs[i]));
      stm32n6_aton_latch_report();

      stm32n6_aton_risaf_clear();
      stm32n6_aton_wd_puts("[probe] addr=0x");
      stm32n6_aton_wd_hex(addrs[i]);
      stm32n6_aton_wd_puts(" ns =");
      stm32n6_aton_wd_hex(stm32n6_aton_ns_read32(1, addrs[i]));
      stm32n6_aton_latch_report();
    }

  stm32n6_aton_risaf_clear();
}

/****************************************************************************
 * Name: stm32n6_aton_npu_state_dump
 *
 * Description:
 *   Dump the epoch controller state machine - the only window into what the
 *   NPU itself is doing while the CPU sits in aton_osal_nuttx_wfe().
 *
 *     CTRL.RUNNING (bit 31)  the epoch controller is executing the blob
 *     ADDR                   blob fetch address the runtime handed over
 *     IRQ                    latched errors (bit1 ERR_BUSPORT, bit3
 *                            ERR_START, bit4 ERR_TIMEOUT, ...)
 *     LABEL                  current epoch label
 *     BC                     blob opcode counter (advances while running)
 *     ACC                    accumulator
 *
 ****************************************************************************/

static void stm32n6_aton_npu_state_dump(void)
{
  stm32n6_aton_gate_dump();
  stm32n6_aton_wd_puts("[npu] ctrl=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_CTRL));
  stm32n6_aton_wd_puts(" addr=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_ADDR));
  stm32n6_aton_wd_puts(" irq=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_IRQ));
  stm32n6_aton_wd_puts(" label=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_LABEL));
  stm32n6_aton_wd_puts(" bc=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_BC));
  stm32n6_aton_wd_puts(" acc=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_ACC));
  stm32n6_aton_wd_puts(" cc=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_CIDCACHE));
  stm32n6_aton_wd_puts("\r\n");
}

/* The stream engines move tensors between memory and the accelerator
 * pipeline.  CTRL says whether one was started (EN) and whether it still
 * runs; ADDR is its current address.  A running engine pointing outside the
 * model's buffers pinpoints where the dataflow is stuck.
 */

/* Host view of the central stream switch: CTRL/VERSION plus every per
 * destination routing register.  The frame start tokens that unblock the
 * stream engines come from here, so an all-zero destination register tells
 * us that the dataflow was never wired up.
 */

/* Full register file of one stream engine.  Besides CTRL/ADDR this holds the
 * frame geometry the engine needs to know when a frame is complete (FSIZE,
 * DEPTH, STRD), the engine's own error flags (IRQ: ILLCFG/FMTMM/limiters -
 * silent unless enabled in EVENT), the frame synchronization setup
 * (EVENT.FRMTRG_EN, EVENT.FRMTRG_SRC: which engine supplies the frame
 * trigger), the external sync mode (EXTSYNC) and the progress counters
 * (FCNT/LINECNT/PIXCNT) that say whether anything moved at all.
 */

static void stm32n6_aton_npu_engine_dump(int e)
{
  uintptr_t b = STM32_ATON_STRENG_CTRL(e);

  stm32n6_aton_wd_puts("[str");
  stm32n6_aton_wd_u32((uint32_t)e);
  stm32n6_aton_wd_puts("] ctrl=");
  stm32n6_aton_wd_hex(getreg32(b + 0x00));
  stm32n6_aton_wd_puts(" fsize=");
  stm32n6_aton_wd_hex(getreg32(b + 0x0c));
  stm32n6_aton_wd_puts(" depth=");
  stm32n6_aton_wd_hex(getreg32(b + 0x10));
  stm32n6_aton_wd_puts(" strd=");
  stm32n6_aton_wd_hex(getreg32(b + 0x14));
  stm32n6_aton_wd_puts(" pos=");
  stm32n6_aton_wd_hex(getreg32(b + 0x24));

  stm32n6_aton_wd_puts("\r\n[str");
  stm32n6_aton_wd_u32((uint32_t)e);
  stm32n6_aton_wd_puts("] event=");
  stm32n6_aton_wd_hex(getreg32(b + 0x28));
  stm32n6_aton_wd_puts(" irq=");
  stm32n6_aton_wd_hex(getreg32(b + 0x3c));
  stm32n6_aton_wd_puts(" extsync=");
  stm32n6_aton_wd_hex(getreg32(b + 0x4c));
  stm32n6_aton_wd_puts(" extsync2=");
  stm32n6_aton_wd_hex(getreg32(b + 0x50));
  stm32n6_aton_wd_puts(" limen=");
  stm32n6_aton_wd_hex(getreg32(b + 0x30));
  stm32n6_aton_wd_puts(" cidc=");
  stm32n6_aton_wd_hex(getreg32(b + 0x48));

  stm32n6_aton_wd_puts("\r\n[str");
  stm32n6_aton_wd_u32((uint32_t)e);
  stm32n6_aton_wd_puts("] fcnt=");
  stm32n6_aton_wd_hex(getreg32(b + 0x68));
  stm32n6_aton_wd_puts(" lcnt=");
  stm32n6_aton_wd_hex(getreg32(b + 0x64));
  stm32n6_aton_wd_puts(" pcnt=");
  stm32n6_aton_wd_hex(getreg32(b + 0x60));
  stm32n6_aton_wd_puts(" last=");
  stm32n6_aton_wd_hex(getreg32(b + 0x58));
  stm32n6_aton_wd_puts(" addr=");
  stm32n6_aton_wd_hex(getreg32(b + 0x08));
  stm32n6_aton_wd_puts(" limit=");
  stm32n6_aton_wd_hex(getreg32(b + 0x34));
  stm32n6_aton_wd_puts(" limad=");
  stm32n6_aton_wd_hex(getreg32(b + 0x38));
  stm32n6_aton_wd_puts(" frpt=");
  stm32n6_aton_wd_hex(getreg32(b + 0x1c));
  stm32n6_aton_wd_puts(" foff=");
  stm32n6_aton_wd_hex(getreg32(b + 0x18));
  stm32n6_aton_wd_puts(" dcnt=");
  stm32n6_aton_wd_hex(getreg32(b + 0x5c));
  stm32n6_aton_wd_puts("\r\n");
}

/* Compact one-line-per-engine view of where the stream engines were pointed.
 * The engine that is supposed to fetch the application's input buffer must show
 * 0x243a2000 in `last`; anything else means the registered input never entered
 * the pipeline, which is exactly what an output that ignores the frame looks
 * like from the outside.
 */

static void stm32n6_aton_streng_sweep(void)
{
  int e;

  for (e = 0; e < 10; e++)
    {
      uintptr_t b = STM32_ATON_STRENG_CTRL(e);

      stm32n6_aton_wd_puts("[strsweep] e=");
      stm32n6_aton_wd_u32((uint32_t)e);
      stm32n6_aton_wd_puts(" ctrl=");
      stm32n6_aton_wd_hex(getreg32(b + 0x00));
      stm32n6_aton_wd_puts(" addr=");
      stm32n6_aton_wd_hex(getreg32(b + 0x08));
      stm32n6_aton_wd_puts(" last=");
      stm32n6_aton_wd_hex(getreg32(b + 0x58));
      stm32n6_aton_wd_puts(" fsz=");
      stm32n6_aton_wd_hex(getreg32(b + 0x0c));
      stm32n6_aton_wd_puts(" pcnt=");
      stm32n6_aton_wd_hex(getreg32(b + 0x60));
      stm32n6_aton_wd_puts("\r\n");
    }
}

/* One compact progress sample of both running engines: pixel/frame counters,
 * the address of the last bus transaction and the engine error flags.  Taken
 * every heartbeat it separates "frozen" from "crawling", and the rate probe
 * below measures how fast it actually crawls.
 */

/* Did the NPU actually write results?  Read the model output buffer through
 * both of its aliases and the tail of what the output stream engine claims to
 * have written, so the data on the memory side can be compared with the
 * progress counters on the unit side.
 */

static void stm32n6_aton_npu_mem_dump(void)
{
  int i;

  stm32n6_aton_wd_puts("[mem] in @24340000:");
  for (i = 0; i < 4; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(0x24340000u + 4u * (uint32_t)i));
    }

  stm32n6_aton_wd_puts("\r\n[mem] out@34280000:");
  for (i = 0; i < 4; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(0x34280000u + 4u * (uint32_t)i));
    }

  stm32n6_aton_wd_puts("\r\n[mem] out@24280000:");
  for (i = 0; i < 4; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(0x24280000u + 4u * (uint32_t)i));
    }

  /* Around the address the output engine stopped at (0x34280000 + 0x1918) */

  stm32n6_aton_wd_puts("\r\n[mem] out@34281900:");
  for (i = 0; i < 8; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(0x34281900u + 4u * (uint32_t)i));
    }

  stm32n6_aton_wd_puts("\r\n");
}

/* Full arithmetic unit register file: besides the coefficients these hold
 * the address generator counters (INCCNT, RSTCNT1-3) and the coefficient
 * memory pointer (COEFFADDR) - "the address of the coeffs to be read next".
 */

static void stm32n6_aton_npu_arith_full(int n)
{
  uintptr_t b = STM32_ATON_BLK_ARITH + (uintptr_t)n * 0x1000;

  stm32n6_aton_wd_puts("[arith");
  stm32n6_aton_wd_u32((uint32_t)n);
  stm32n6_aton_wd_puts("] ctrl=");
  stm32n6_aton_wd_hex(getreg32(b + 0x00));
  stm32n6_aton_wd_puts(" sh=");
  stm32n6_aton_wd_hex(getreg32(b + 0x08));
  stm32n6_aton_wd_puts(" incc=");
  stm32n6_aton_wd_hex(getreg32(b + 0x0c));
  stm32n6_aton_wd_puts(" rst1=");
  stm32n6_aton_wd_hex(getreg32(b + 0x10));
  stm32n6_aton_wd_puts(" rst2=");
  stm32n6_aton_wd_hex(getreg32(b + 0x14));
  stm32n6_aton_wd_puts(" rst3=");
  stm32n6_aton_wd_hex(getreg32(b + 0x18));

  stm32n6_aton_wd_puts("\r\n[arith");
  stm32n6_aton_wd_u32((uint32_t)n);
  stm32n6_aton_wd_puts("] aoff=");
  stm32n6_aton_wd_hex(getreg32(b + 0x24));
  stm32n6_aton_wd_puts(" ioff=");
  stm32n6_aton_wd_hex(getreg32(b + 0x28));
  stm32n6_aton_wd_puts(" xlate=");
  stm32n6_aton_wd_hex(getreg32(b + 0x2c));
  stm32n6_aton_wd_puts(" coffa=");
  stm32n6_aton_wd_hex(getreg32(b + 0x30));
  stm32n6_aton_wd_puts(" insh=");
  stm32n6_aton_wd_hex(getreg32(b + 0x34));
  stm32n6_aton_wd_puts(" clip=");
  stm32n6_aton_wd_hex(getreg32(b + 0x38));
  stm32n6_aton_wd_puts(" ca=");
  stm32n6_aton_wd_hex(getreg32(b + 0x1c));
  stm32n6_aton_wd_puts(" cb=");
  stm32n6_aton_wd_hex(getreg32(b + 0x20));
  stm32n6_aton_wd_puts("\r\n");
}

/* Debug/trace unit: enable it and look at every register, including the one
 * at 0x18 that the header does not name.  Its stream switch link monitors
 * are the only window into which link of the dataflow fabric is blocked.
 */

static void stm32n6_aton_npu_trace_probe(void)
{
  int i;

  putreg32(1u, STM32_ATON_TRACE_CTRL);
  nxsig_usleep(200000);

  stm32n6_aton_wd_puts("[trace2] en:");
  for (i = 0; i < 16; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(STM32_NPU_BASE + 0x1f000u
                                   + 4u * (uint32_t)i));
    }

  stm32n6_aton_wd_puts("\r\n");
}

/* Stop the epoch controller and re-run the very same blob one micro
 * instruction at a time.  If the counter then walks past the index it is
 * stuck on, the stall is a wait for something outside the instruction
 * stream; if it stops at the same place, that micro instruction is the one
 * waiting.
 */

/* Deadlock probe: the epoch controller executes micro instructions that
 * program unit registers, and most unit registers are read-only while the
 * unit is running ("RO when CTRL.RUNNING").  If the counter does not move
 * because the controller is waiting for such a write to take effect, taking
 * the units out of their running state must make it advance again.
 *
 * So: stop both stream engines, watch the counter, clear their pipelines,
 * watch again, and finally put them back the way they were.
 */

/* Second round of pokes.
 *
 * (a) The trace unit's link monitors only count once the unit is enabled, so
 *     sample them repeatedly: the bits that change name the links that are
 *     stalling while the dataflow is frozen.
 *
 * (b) The currently frozen unit set is the four units the model uses.  Enable
 *     every other functional unit (all read CTRL == 0) in case the epoch
 *     program is waiting for one of them, and watch the counter.
 *
 * (c) Toggle the stream switch and the two bus interfaces, again watching the
 *     counter: a controller waiting for one of them would move on.
 */

static void stm32n6_aton_npu_poke2(void)
{
  static const uint32_t blk[11] =
  {
    STM32_ATON_BLK_STRENG + 0 * 0x1000,
    STM32_ATON_BLK_STRENG + 1 * 0x1000,
    STM32_ATON_BLK_STRENG + 2 * 0x1000,
    STM32_ATON_BLK_STRENG + 4 * 0x1000,
    STM32_ATON_BLK_CONVACC + 0 * 0x1000,
    STM32_ATON_BLK_DECUN + 0 * 0x1000,
    STM32_ATON_BLK_ACTIV + 0 * 0x1000,
    STM32_ATON_BLK_POOL + 0 * 0x1000,
    STM32_ATON_BLK_RECBUF,
    STM32_ATON_BLK_ARITH + 2 * 0x1000,
    STM32_ATON_BLK_ARITH + 3 * 0x1000
  };

  int i;

  /* (a) link monitor deltas */

  putreg32(1u, STM32_ATON_TRACE_CTRL);

  for (i = 0; i < 3; i++)
    {
      stm32n6_aton_wd_puts("[lnk] il=");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_TRACE_ILINK_LO));
      stm32n6_aton_wd_puts(" ih=");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_TRACE_ILINK_HI));
      stm32n6_aton_wd_puts(" ol=");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_TRACE_OLINK_LO));
      stm32n6_aton_wd_puts(" oh=");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_TRACE_OLINK_HI));
      stm32n6_aton_wd_puts("\r\n");

      nxsig_usleep(200000);
    }

  /* (b) enable the idle units */

  for (i = 0; i < 11; i++)
    {
      putreg32(1u, blk[i]);
    }

  nxsig_usleep(200000);
  stm32n6_aton_wd_puts("[poke2] units enabled, bc=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_BC));
  stm32n6_aton_wd_puts(" irq=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_IRQ));
  stm32n6_aton_wd_puts("\r\n");

  /* (c) switch and bus interfaces */

  putreg32(0, STM32_ATON_STRSWITCH_CTRL);
  nxsig_usleep(200000);
  stm32n6_aton_wd_puts("[poke2] switch off, bc=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_BC));
  stm32n6_aton_wd_puts("\r\n");

  putreg32(1u, STM32_ATON_STRSWITCH_CTRL);
  nxsig_usleep(200000);
  stm32n6_aton_wd_puts("[poke2] switch on, bc=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_BC));
  stm32n6_aton_wd_puts(" dst9=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRSWITCH_DST(9)));
  stm32n6_aton_wd_puts(" dst28=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRSWITCH_DST(28)));
  stm32n6_aton_wd_puts("\r\n");
}

static void stm32n6_aton_npu_deadlock_probe(void)
{
  static const int units[4] =
  {
    STM32_ATON_BLK_STRENG + 3 * 0x1000,   /* input stream engine  */
    STM32_ATON_BLK_STRENG + 9 * 0x1000,   /* output stream engine */
    STM32_ATON_BLK_ARITH + 0,             /* arithmetic slice 0   */
    STM32_ATON_BLK_ARITH + 0x1000         /* arithmetic slice 1   */
  };

  int i;

  for (i = 0; i < 4; i++)
    {
      uint32_t before = getreg32(STM32_ATON_EC_BC);

      stm32n6_aton_wd_puts("[poke] unit=");
      stm32n6_aton_wd_hex((uint32_t)(units[i] - STM32_NPU_BASE));
      stm32n6_aton_wd_puts(" ctrl=");
      stm32n6_aton_wd_hex(getreg32(units[i]));
      stm32n6_aton_wd_puts(" bc=");
      stm32n6_aton_wd_hex(before);

      putreg32(0, units[i]);
      nxsig_usleep(100000);

      stm32n6_aton_wd_puts(" -> bc=");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_BC));
      stm32n6_aton_wd_puts(" ecctrl=");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_CTRL));
      stm32n6_aton_wd_puts(" ecirq=");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_IRQ));
      stm32n6_aton_wd_puts("\r\n");
    }
}

static void stm32n6_aton_npu_step_probe(void)
{
  int i;

  putreg32(0, STM32_ATON_EC_CTRL);
  stm32n6_aton_wd_puts("[step] stop ctrl=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_CTRL));
  stm32n6_aton_wd_puts(" irq=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_IRQ));
  stm32n6_aton_wd_puts(" bc=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_BC));

  putreg32(0, STM32_ATON_EC_IRQ);

  /* SM (step mode) = bit 3, EN = bit 0; ADDR still points at the blob */

  putreg32(1u | (1u << 3), STM32_ATON_EC_CTRL);
  stm32n6_aton_wd_puts("\r\n[step] armed ctrl=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_CTRL));
  stm32n6_aton_wd_puts(" addr=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_ADDR));

  for (i = 0; i < 64; i++)
    {
      putreg32(1u << 16, STM32_ATON_EC_IRQ);

      if ((i % 8) == 7)
        {
          stm32n6_aton_wd_puts("\r\n[step] i=");
          stm32n6_aton_wd_u32((uint32_t)(i + 1));
          stm32n6_aton_wd_puts(" bc=");
          stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_BC));
          stm32n6_aton_wd_puts(" irq=");
          stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_IRQ));
        }
    }

  stm32n6_aton_wd_puts("\r\n");
}

static void stm32n6_aton_npu_progress_puts(int e)
{
  uintptr_t b = STM32_ATON_STRENG_CTRL(e);

  stm32n6_aton_wd_puts(" s");
  stm32n6_aton_wd_u32((uint32_t)e);
  stm32n6_aton_wd_puts("=");
  stm32n6_aton_wd_hex(getreg32(b + 0x60));
  stm32n6_aton_wd_puts("/");
  stm32n6_aton_wd_hex(getreg32(b + 0x58));
  stm32n6_aton_wd_puts("/");
  stm32n6_aton_wd_hex(getreg32(b + 0x68));
  stm32n6_aton_wd_puts("/");
  stm32n6_aton_wd_hex(getreg32(b + 0x3c));
}

static void stm32n6_aton_npu_progress_dump(void)
{
  stm32n6_aton_wd_puts("[prog] bc=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_EC_BC));
  stm32n6_aton_npu_progress_puts(3);
  stm32n6_aton_npu_progress_puts(9);
  stm32n6_aton_wd_puts("\r\n");
}

/* Rate probe: clear the engine error flags, then take three samples a fixed
 * delay apart.  The pixel counter deltas give the dataflow throughput in
 * pixels per sample window - a trickle points at a bandwith/throttle limit,
 * a standstill at a blocked (or dead) unit.
 */

static void stm32n6_aton_npu_rate_probe(void)
{
  int i;

  /* Acknowledge the flags we saw so that only new events show up again */

  putreg32(getreg32(STM32_ATON_STRENG_CTRL(3) + 0x3c),
           STM32_ATON_STRENG_CTRL(3) + 0x3c);
  putreg32(getreg32(STM32_ATON_STRENG_CTRL(9) + 0x3c),
           STM32_ATON_STRENG_CTRL(9) + 0x3c);

  for (i = 0; i < 3; i++)
    {
      stm32n6_aton_wd_puts("[rate]");
      stm32n6_aton_npu_progress_puts(3);
      stm32n6_aton_npu_progress_puts(9);
      stm32n6_aton_wd_puts("\r\n");

      nxsig_usleep(300000);
    }
}

/* Arithmetic unit registers: the two slices that the epoch program wired
 * between the input and the output stream engine.
 */

static void stm32n6_aton_npu_arith_dump(int n)
{
  uintptr_t b = STM32_ATON_BLK_ARITH + (uintptr_t)n * 0x1000;

  stm32n6_aton_wd_puts("[arith");
  stm32n6_aton_wd_u32((uint32_t)n);
  stm32n6_aton_wd_puts("] ctrl=");
  stm32n6_aton_wd_hex(getreg32(b + 0x00));
  stm32n6_aton_wd_puts(" shift=");
  stm32n6_aton_wd_hex(getreg32(b + 0x08));
  stm32n6_aton_wd_puts(" inccnt=");
  stm32n6_aton_wd_hex(getreg32(b + 0x0c));
  stm32n6_aton_wd_puts(" cA=");
  stm32n6_aton_wd_hex(getreg32(b + 0x1c));
  stm32n6_aton_wd_puts(" cB=");
  stm32n6_aton_wd_hex(getreg32(b + 0x20));
  stm32n6_aton_wd_puts(" xlate=");
  stm32n6_aton_wd_hex(getreg32(b + 0x2c));
  stm32n6_aton_wd_puts(" coffa=");
  stm32n6_aton_wd_hex(getreg32(b + 0x30));
  stm32n6_aton_wd_puts(" insh=");
  stm32n6_aton_wd_hex(getreg32(b + 0x34));
  stm32n6_aton_wd_puts(" clip=");
  stm32n6_aton_wd_hex(getreg32(b + 0x38));
  stm32n6_aton_wd_puts("\r\n");
}

static void stm32n6_aton_npu_switch_dump(void)
{
  uint32_t ctrl = getreg32(STM32_ATON_STRSWITCH_CTRL);
  int      i;

  stm32n6_aton_wd_puts("[switch] ctrl=");
  stm32n6_aton_wd_hex(ctrl);
  stm32n6_aton_wd_puts(" ver=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRSWITCH_VERSION));

  stm32n6_aton_wd_puts("\r\n[switch] dst:");
  for (i = 0; i < STM32_ATON_STRSWITCH_NDST; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRSWITCH_DST(i)));
    }

  stm32n6_aton_wd_puts("\r\n");

  /* CTRL.EN gates the whole switch: with it cleared no frame start can be
   * generated and every stream engine stalls in "wait for frame start".
   * Turn it on (and say so) so the next heartbeats show whether that alone
   * sets the dataflow in motion.
   */

  if ((ctrl & 1u) == 0)
    {
      putreg32(ctrl | 1u, STM32_ATON_STRSWITCH_CTRL);
      stm32n6_aton_wd_puts("[switch] EN was 0 -> set to 1, ctrl now=");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRSWITCH_CTRL));
      stm32n6_aton_wd_puts("\r\n");
    }
}

static void stm32n6_aton_npu_streng_dump(void)
{
  int i;

  stm32n6_aton_wd_puts("[streng] ctrl:");
  for (i = 0; i < STM32_ATON_NSTRENG; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRENG_CTRL(i)));
    }

  stm32n6_aton_wd_puts("\r\n[streng] addr:");
  for (i = 0; i < STM32_ATON_NSTRENG; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(STM32_ATON_STRENG_ADDR(i)));
    }

  stm32n6_aton_wd_puts("\r\n");
}

/* Which unit currently asks for attention (the host reads INTREG to find
 * out), how the runtime routed unit interrupts, and what the debug/trace
 * unit sees on the stream switch links.
 */

static void stm32n6_aton_npu_irq_trace_dump(void)
{
  stm32n6_aton_wd_puts("[int] intreg=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_INTCTRL_INTREG));
  stm32n6_aton_wd_puts(" or0=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_INTCTRL_ORMSK0));
  stm32n6_aton_wd_puts(" or1=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_INTCTRL_ORMSK1));
  stm32n6_aton_wd_puts(" and0=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_INTCTRL_ANDMSK0));
  stm32n6_aton_wd_puts(" and1=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_INTCTRL_ANDMSK1));
  stm32n6_aton_wd_puts("\r\n");

  stm32n6_aton_wd_puts("[trace] ctrl=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_TRACE_CTRL));
  stm32n6_aton_wd_puts(" il=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_TRACE_ILINK_LO));
  stm32n6_aton_wd_puts(" ih=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_TRACE_ILINK_HI));
  stm32n6_aton_wd_puts(" ol=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_TRACE_OLINK_LO));
  stm32n6_aton_wd_puts(" oh=");
  stm32n6_aton_wd_hex(getreg32(STM32_ATON_TRACE_OLINK_HI));
  stm32n6_aton_wd_puts("\r\n");
}

/* The epoch controller executes micro instructions directly out of the blob
 * at ADDR and BC counts its way through that image: dump the header and the
 * words around BC, so the blocking micro instruction can be looked at.
 */

static void stm32n6_aton_npu_blob_dump(void)
{
  uint32_t base = (uint32_t)getreg32(STM32_ATON_EC_ADDR);
  uint32_t bc   = (uint32_t)getreg32(STM32_ATON_EC_BC);
  uint32_t idx;
  int      i;

  /* Read the program through a fresh cache state: a stale D-cache line looks
   * like garbage that is not actually in RAM.
   */

  /* r60: EC_ADDR reads back as zero once the runtime has de-initialised, and
   * the r59 log ended in a precise bus fault (CFSR 0x8200, BFAR 0x4) because
   * this dump then dereferenced address zero.  Only touch memory that really
   * lies inside the blob window; anything else is reported and skipped.
   */

  if (base < STM32N6_ATON_NS_WINDOW_BASE ||
      base >= STM32N6_ATON_NS_WINDOW_BASE + STM32N6_ATON_NS_WINDOW_SIZE)
    {
      stm32n6_aton_wd_puts("[blob] base=0x");
      stm32n6_aton_wd_hex(base);
      stm32n6_aton_wd_puts(" (outside the blob window, dump skipped)\r\n");
      return;
    }

  /* The BC-relative reads must stay inside the window as well. */

  if (4u * (bc * 2u + 8u) >
      STM32N6_ATON_NS_WINDOW_BASE + STM32N6_ATON_NS_WINDOW_SIZE - base)
    {
      stm32n6_aton_wd_puts("[blob] base=0x");
      stm32n6_aton_wd_hex(base);
      stm32n6_aton_wd_puts(" bc=0x");
      stm32n6_aton_wd_hex(bc);
      stm32n6_aton_wd_puts(" (bc beyond the window, word dump skipped)\r\n");
      return;
    }

  up_invalidate_dcache((uintptr_t)base, (uintptr_t)base + 64);

  stm32n6_aton_wd_puts("[blob] base=");
  stm32n6_aton_wd_hex(base);
  stm32n6_aton_wd_puts(" bc=");
  stm32n6_aton_wd_hex(bc);
  stm32n6_aton_wd_puts(" hdr:");
  for (i = 0; i < 4; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(base + 4u * (uint32_t)i));
    }

  /* Words around the counter, read as 32 bit words and as the 64 bit lines
   * the epoch controller fetches (the counter may count either of them).
   */

  stm32n6_aton_wd_puts("\r\n[blob] w32:");
  for (i = -2; i <= 3; i++)
    {
      idx = (uint32_t)((int32_t)bc + i);
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(base + 4u * idx));
    }

  stm32n6_aton_wd_puts("\r\n[blob] w64:");
  for (i = 0; i < 6; i++)
    {
      idx = bc * 2u + (uint32_t)i;
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32(base + 4u * idx));
    }

  stm32n6_aton_wd_puts("\r\n");
}

static int stm32n6_aton_watchdog(int argc, FAR char *argv[])
{
  bool dumped = false;
  uint32_t last_bc = 0;
  int  stall_samples = 0;

  UNUSED(argc);
  UNUSED(argv);

  for (;;)
    {
      int ret = nxsig_usleep(ATON_WD_PERIOD_US);
      if (ret < 0)
        {
          /* Never turn a failed sleep into a busy loop */

          sched_yield();
        }

      if (aton_model_storm_masked() && !dumped)
        {
          dumped = true;

          _err("ERROR: NPU interrupt line masked (interrupt storm); "
               "inference will not complete. Runtime log follows:\n");
          aton_model_dump_log();
          stm32n6_aton_rif_fault_dump();
        }

      /* Between runs the NPU is idle, so the two-second heartbeat, the NPU
       * register dump and the task list are pure noise - and they cost UART
       * bandwidth right where the next run wants it.  Talk only while a run is
       * in flight, or once a storm has been detected.
       */

      if (!g_aton_run_active && !aton_model_storm_masked())
        {
          continue;
        }

      stm32n6_aton_wd_puts("[aton_wd] tick=");
      stm32n6_aton_wd_u32((uint32_t)clock_systime_ticks());
      stm32n6_aton_wd_puts(" run=");
      stm32n6_aton_wd_u32(g_aton_run_active ? 1 : 0);
      stm32n6_aton_wd_puts(" irq=");
      stm32n6_aton_wd_u32(aton_model_irq_count());
      stm32n6_aton_wd_puts(" stage=");
      stm32n6_aton_wd_u32((uint32_t)aton_model_stage());
      stm32n6_aton_wd_puts(" storm=");
      stm32n6_aton_wd_u32(aton_model_storm_masked() ? 1 : 0);
      stm32n6_aton_wd_puts(" primask=");
      stm32n6_aton_wd_u32(stm32n6_aton_wd_primask());
      stm32n6_aton_wd_puts(" basepri=");
      stm32n6_aton_wd_u32(stm32n6_aton_wd_basepri());
      stm32n6_aton_wd_puts("\r\n");

      /* While the runtime waits for the NPU, ask the NPU itself.  RUNNING
       * says whether it ever started, ADDR which blob it was pointed at,
       * BC whether it is making progress, IRQ what it complained about.
       */

      if (aton_model_stage() == STM32N6_ATON_STAGE_WFE)
        {
          uint32_t bc = (uint32_t)getreg32(STM32_ATON_EC_BC);

          /* Progress watchdog: BC (the blob opcode counter) advancing means the
           * epoch controller is working - the runtime is simply waiting for the
           * next NPU event, which is what every healthy run looks like.  Only a
           * BC that stops moving is a stall, and only then is the intrusive
           * half of the diagnostics worth its cost.
           */

          if (bc == last_bc)
            {
              stall_samples++;
            }
          else
            {
              stall_samples = 0;
            }

          last_bc = bc;

          /* Cheap, non-intrusive view, once per sample. */

          stm32n6_aton_npu_progress_dump();
          stm32n6_aton_npu_state_dump();
          stm32n6_aton_npu_bus_dump();
          stm32n6_aton_npu_block_dump();

          /* r60: the intrusive dump (it pokes unit registers and dereferences
           * EC_ADDR) used to run on the first WFE sample of every run.  That
           * flooded the log during healthy inferences, disturbed the units and
           * - with EC_ADDR cleared after a finished run - ended the r59 log in
           * a precise bus fault (BFAR = 0x4).  Now it needs a real stall.
           */

          if (stall_samples >= STM32N6_ATON_STALL_SAMPLES && !dumped)
            {
              dumped = true;

              stm32n6_aton_npu_clocks_on();
              stm32n6_aton_npu_streng_dump();
              stm32n6_aton_npu_mem_dump();
              stm32n6_aton_npu_rate_probe();
              stm32n6_aton_npu_engine_dump(3);
              stm32n6_aton_npu_engine_dump(9);
              stm32n6_aton_npu_arith_full(0);
              stm32n6_aton_npu_arith_full(1);
              stm32n6_aton_npu_switch_dump();
              stm32n6_aton_npu_deadlock_probe();
              stm32n6_aton_npu_poke2();
              stm32n6_aton_npu_irq_trace_dump();
              stm32n6_aton_npu_blob_dump();
              stm32n6_aton_risaf_xspi_dump();
              stm32n6_aton_rif_fault_dump();
                        }
        }

      /* Task states and the console transmitter state: together they say
       * whether the application is blocked (and on what) or still running,
       * and whether the console TX path is the thing that stopped.
       */

      stm32n6_aton_wd_puts("[tasks]");
      nxsched_foreach(stm32n6_aton_wd_task, NULL);
      stm32n6_aton_wd_puts("\r\n");

      /* A lost console wakeup cannot be reported by the console itself: the
       * serial driver's TX interrupt is re-armed from here (see
       * stm32n6_serial.c) and its transmitter state is printed with the raw
       * channel, so a shell that went quiet can be told apart from a system
       * that stopped running.
       */

      stm32n6_console_unstick();
      stm32n6_console_dump();

      if (g_aton_run_active)
        {
          aton_model_report("running");
        }
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_aton_publish_model
 *
 * Description:
 *   Register the buffer the application fills for slot id and publish that
 *   slot's epoch program.
 *
 *   The order is not negotiable.  The program carries the input address as a
 *   relocation entry that ec_reloc() resolves once, while the program is
 *   published; a buffer registered afterwards never reaches the engines, and
 *   the symptom is an inference that returns a plausible answer forever while
 *   ignoring every frame (r56 lost an afternoon to exactly that).
 *
 *   Each model has its own loader - the generated symbol carries the model
 *   suffix - and the generated NN_Interface_<suffix> object holds a pointer to
 *   it, so the call goes through aton_model_publish() instead of naming one
 *   model's loader from here.
 *
 * Returned Value:
 *   OK on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int
stm32n6_aton_publish_model(FAR struct stm32n6_aton_lowerhalf_s *priv, int id)
{
  FAR struct stm32n6_aton_model_s *m = &priv->model[id];
  uint32_t in_len  = aton_model_input_len(id);
  uint32_t out_len = aton_model_output_len(id);
  int ret;

  /* A model whose tensor does not fit the shared buffer would be published
   * with an address that partly overlaps whatever comes next in the window,
   * and the engines would read that.  Refuse it here, at init, instead of
   * letting it produce wrong answers later.
   */

  if (in_len > STM32N6_ATON_APP_INPUT_SIZE)
    {
      _err("ERROR: ATON model %d needs %lu bytes of input but the shared "
           "buffer holds %u\n", id, (unsigned long)in_len,
           (unsigned)STM32N6_ATON_APP_INPUT_SIZE);
      return -ENOMEM;
    }

  ret = aton_model_set_input_buffer(id,
                                    (FAR void *)STM32N6_ATON_APP_INPUT_BASE,
                                    in_len);
  if (ret < 0)
    {
      _err("ERROR: ATON model %d rejected the input buffer: %d\n", id, ret);
      return ret;
    }

  m->input_addr  = STM32N6_ATON_APP_INPUT_BASE;
  m->input_size  = in_len;
  m->output_size = out_len;
  m->output_addr = (uint32_t)aton_model_output_addr(id);

  stm32n6_aton_wd_puts("[in] model ");
  stm32n6_aton_wd_u32((uint32_t)id);
  stm32n6_aton_wd_puts(" register 0x");
  stm32n6_aton_wd_hex(m->input_addr);
  stm32n6_aton_wd_puts(" len=");
  stm32n6_aton_wd_u32(in_len);
  stm32n6_aton_wd_puts(" program_uses=");
  stm32n6_aton_wd_hex((uint32_t)aton_model_input_pointer(id));
  stm32n6_aton_wd_puts("\r\n");

  /* Publish: the loader reads the blob out of .nsblob, so the relocated
   * copies have to be in RAM already - aton_model_blob_sync() ran above.
   */

  ret = aton_model_publish(id);
  m->published = (ret == OK);

  stm32n6_aton_wd_puts("[ec] model ");
  stm32n6_aton_wd_u32((uint32_t)id);
  stm32n6_aton_wd_puts(" publish=");
  stm32n6_aton_wd_u32(m->published ? 1 : 0);
  stm32n6_aton_wd_puts("\r\n");

  return ret;
}

/****************************************************************************
 * Name: stm32n6_aton_aie_init
 *
 * Description:
 *   Start a session (AIE_CMD_LOAD): relocate the secondary blobs into the
 *   non-secure window, initialise the ATON runtime, register the input buffer
 *   the application fills, publish the epoch program and make sure the NPU
 *   sees it in RAM.
 *
 * Returned Value:
 *   The per-session context (the lower-half instance) on success; NULL if the
 *   session could not be started.
 *
 ****************************************************************************/

static FAR void *stm32n6_aton_aie_init(FAR struct aie_lowerhalf_s *lower,
                                       uintptr_t model)
{
  FAR struct stm32n6_aton_lowerhalf_s *priv =
    (FAR struct stm32n6_aton_lowerhalf_s *)lower;
  int  ret;
  int  i;

  /* The model is linked into the image (the ATON runtime is bound to it at
   * build time), so the loader argument is not used.
   */

  UNUSED(model);

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return NULL;
    }

  if (priv->opened)
    {
      nxmutex_unlock(&priv->lock);
      return NULL;
    }

  /* The secondary blobs (everything except the program the epoch controller
   * executes) have to be reachable by the NPU, which cannot see the secure
   * SRAM images: aton_model_blob_sync() copies them into the non-secure
   * window and returns the checksum of what it copied.
   */

  stm32n6_aton_wd_puts("[nsblob] secondary blobs -> 0x");
  stm32n6_aton_wd_hex(STM32N6_ATON_NS_WINDOW_BASE);
  stm32n6_aton_wd_puts("..");
  stm32n6_aton_wd_hex(STM32N6_ATON_NS_WINDOW_BASE +
                      STM32N6_ATON_NS_WINDOW_SIZE);
  stm32n6_aton_wd_puts(" sum=");
  stm32n6_aton_wd_hex(aton_model_blob_sync(ATON_MODEL_PRIMARY));
#if defined(CONFIG_AI_ATON_MODEL_EYE)
  stm32n6_aton_wd_puts(" face=");
  stm32n6_aton_wd_hex(aton_model_blob_sync(ATON_MODEL_FACE));
#endif
  stm32n6_aton_wd_puts(" models=");
  stm32n6_aton_wd_u32((uint32_t)ATON_MODEL_NMODELS);
  stm32n6_aton_wd_puts("\r\n");

  /* The blobs and the application's input buffer share the non-secure window,
   * and nothing in the link checks one against the other: a blob that grew
   * into the buffer would be overwritten by the frame, and the symptom is a
   * program that still runs but reads nonsense.  94% of the window is spoken
   * for, so say so here rather than debug it from a wrong answer.
   */

  if ((uintptr_t)_snsblob + STM32N6_ATON_NS_BLOB_SIZE >
      STM32N6_ATON_APP_INPUT_BASE)
    {
      _err("ERROR: .nsblob ends at 0x%08lx, past the input buffer at "
           "0x%08x\n",
           (unsigned long)((uintptr_t)_snsblob + STM32N6_ATON_NS_BLOB_SIZE),
           (unsigned)STM32N6_ATON_APP_INPUT_BASE);
      nxmutex_unlock(&priv->lock);
      return NULL;
    }

  ret = aton_model_init(ATON_MODEL_PRIMARY);
  if (ret < 0)
    {
      nxmutex_unlock(&priv->lock);
      return NULL;
    }

#if defined(CONFIG_AI_ATON_MODEL_EYE)
  /* Bring the second model up here as well.  Its tables are read for the
   * first time at this instant, so a binding that lost its network - the
   * failure mode that costs an afternoon when it only shows up as a wrong
   * answer - becomes a boot-time error instead.
   */

  ret = aton_model_init(ATON_MODEL_FACE);
  if (ret < 0)
    {
      nxmutex_unlock(&priv->lock);
      return NULL;
    }
#endif

  aton_osal_nuttx_init();

  /* Every slot gets its input buffer registered and its program published,
   * in that order (see stm32n6_aton_publish_model).
   */

  ret = stm32n6_aton_publish_model(priv, ATON_MODEL_PRIMARY);
  if (ret < 0)
    {
      nxmutex_unlock(&priv->lock);
      return NULL;
    }

#if defined(CONFIG_AI_ATON_MODEL_EYE)
  ret = stm32n6_aton_publish_model(priv, ATON_MODEL_FACE);
  if (ret < 0)
    {
      nxmutex_unlock(&priv->lock);
      return NULL;
    }
#endif

  /* The NPU is not coherent with the CPU data cache: clean the freshly
   * written blobs out to RAM and drop the stale line for the loader word
   * before the epoch controller fetches them.
   *
   * STM32N6_ATON_NS_BLOB_SIZE is the size of the .nsblob section (0x7620 as
   * reported by the map file).
   */

  up_clean_dcache((uintptr_t)_snsblob,
                  (uintptr_t)_snsblob + STM32N6_ATON_NS_BLOB_SIZE);
  up_invalidate_dcache((uintptr_t)_snsblob, (uintptr_t)_snsblob + 32);

  /* The loader word and the program header: the four-byte loader field is 1
   * once the program was accepted, and the header words identify the blob.
   */

  stm32n6_aton_wd_puts("[blob-pre] img:");
  for (i = 0; i < 8; i++)
    {
      stm32n6_aton_wd_puts(" ");
      stm32n6_aton_wd_hex(getreg32((uint32_t)(uintptr_t)_snsblob +
                                   4u * (uint32_t)i));
    }

  stm32n6_aton_wd_puts("\r\n");

  /* One watchdog for the whole system: it is started with the first session
   * and lives in the KDMMY (kernel) TCB, so it survives the application.
   */

  if (!g_aton_wd_started)
    {
      g_aton_wd_started = true;

      ret = kthread_create("aton_wd", ATON_WD_PRIORITY, ATON_WD_STACKSIZE,
                           stm32n6_aton_watchdog, NULL);
      if (ret < 0)
        {
          _err("ERROR: failed to start ATON watchdog: %d\n", ret);
        }
    }

  priv->opened = true;

  nxmutex_unlock(&priv->lock);

  return lower;
}

/****************************************************************************
 * Name: stm32n6_aton_aie_deinit
 *
 * Description:
 *   End a session (close of the AIE device).  The runtime is left loaded: the
 *   epoch program and the relocated blobs stay in the non-secure window, so a
 *   later session only has to start the epoch loop again.
 *
 ****************************************************************************/

static int stm32n6_aton_aie_deinit(FAR struct aie_lowerhalf_s *lower,
                                   FAR void *context)
{
  FAR struct stm32n6_aton_lowerhalf_s *priv =
    (FAR struct stm32n6_aton_lowerhalf_s *)lower;
  int ret;

  UNUSED(context);

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  priv->opened = false;

  nxmutex_unlock(&priv->lock);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_aton_aie_feed_input
 *
 * Description:
 *   Run one inference (AIE_CMD_FEED_INPUT).  The argument is a
 *   struct stm32n6_aton_invoke_params carrying the input and output buffers;
 *   the duration, run and event counters are returned in the same block.
 *
 *   The per-run diagnostics (blob header, RISAF latches, the pointer the
 *   program really uses, the strength register sweep) are what turned the
 *   "every frame gives the same answer" mystery into a one-line answer, so
 *   they stay in the log at this level of detail.
 *
 ****************************************************************************/

static int stm32n6_aton_aie_feed_input(FAR struct aie_lowerhalf_s *lower,
                                       FAR void *context, uintptr_t input)
{
  FAR struct stm32n6_aton_lowerhalf_s *priv =
    (FAR struct stm32n6_aton_lowerhalf_s *)lower;
  FAR struct stm32n6_aton_invoke_params *params;
  FAR struct stm32n6_aton_model_s *m;
  int id;
  int ret = OK;
  int i;

  UNUSED(context);

  if (input == 0)
    {
      return -EINVAL;
    }

  params = (FAR struct stm32n6_aton_invoke_params *)(uintptr_t)input;

  if (params->input == NULL || params->output == NULL)
    {
      return -EINVAL;
    }

  ret = nxmutex_lock(&priv->lock);
  if (ret < 0)
    {
      return ret;
    }

  if (!priv->opened)
    {
      nxmutex_unlock(&priv->lock);
      return -EPERM;
    }

  /* Which model this run is for.  A caller that leaves the field at zero gets
   * the primary model, which is what a caller written against the one-model
   * driver expects.  A build without the second model answers -ENODEV rather
   * than quietly running the first one: a caller that asked for the face
   * detector and got eye classifications would otherwise never find out.
   */

  id = (int)params->model_id;

  if (id < 0 || id >= ATON_MODEL_NMODELS)
    {
      nxmutex_unlock(&priv->lock);
      return -ENODEV;
    }

  m = &priv->model[id];

  if (!m->published)
    {
      nxmutex_unlock(&priv->lock);
      return -ENODEV;
    }

  /* The epoch controller reads the program straight out of the .nsblob
   * section; a stale D-cache line would show words that are not in RAM, so
   * the header is read through a fresh cache state.
   */

  g_aton_perf_c0 = stm32n6_aton_cyccnt();

  up_invalidate_dcache((uintptr_t)_snsblob, (uintptr_t)_snsblob + 32);

  if (g_aton_perf_diag)
    {
      stm32n6_aton_wd_puts("[blob-run] img:");
      for (i = 0; i < 8; i++)
        {
          stm32n6_aton_wd_puts(" ");
          stm32n6_aton_wd_hex(getreg32((uint32_t)(uintptr_t)_snsblob +
                                       4u * (uint32_t)i));
        }

      stm32n6_aton_wd_puts("\r\n");
    }

  /* Old latches would be read back as if this run had produced them. */

  stm32n6_aton_risaf_clear();

  g_aton_run_active = true;
  g_aton_perf_c1 = stm32n6_aton_cyccnt();

  /* r79: the pointer below only reaches the engines when it is the address
   * the program was published with - ec_reloc() resolved that one once.  A
   * caller that filled some other buffer gets the same output for every
   * frame, with nothing anywhere reporting an error, so name it once.
   */

  if (params->input != (FAR const void *)m->input_addr && !m->warned)
    {
      m->warned = true;
      stm32n6_aton_wd_puts("[in] WARNING: model ");
      stm32n6_aton_wd_u32((uint32_t)id);
      stm32n6_aton_wd_puts(" is fed 0x");
      stm32n6_aton_wd_hex((uint32_t)(uintptr_t)params->input);
      stm32n6_aton_wd_puts(" but its program reads 0x");
      stm32n6_aton_wd_hex(m->input_addr);
      stm32n6_aton_wd_puts(" - that data never reaches the NPU\r\n");
    }

  ret = aton_model_run(id, params->input, params->output, &m->last_usec);

  g_aton_perf_c2 = stm32n6_aton_cyccnt();
  g_aton_run_active = false;

  if (g_aton_perf_diag)
    {
      stm32n6_aton_wd_puts("[risaf-run] latches after the run:\r\n");
      stm32n6_aton_rif_fault_dump();

      stm32n6_aton_wd_puts("[in] app=");
      stm32n6_aton_wd_hex((uint32_t)(uintptr_t)params->input);
      stm32n6_aton_wd_puts(" program_uses=");
      stm32n6_aton_wd_hex((uint32_t)aton_model_input_pointer(ATON_MODEL_PRIMARY));
      stm32n6_aton_wd_puts(" table_pointer_at=0x");
      stm32n6_aton_wd_hex((uint32_t)aton_model_input_registered(ATON_MODEL_PRIMARY));
      stm32n6_aton_wd_puts("\r\n");

      stm32n6_aton_streng_sweep();
    }

  g_aton_perf_c3 = stm32n6_aton_cyccnt();

  /* Where the time actually goes.  The NPU phase is measured by the model
   * layer in microseconds; the two CPU phases (blob snapshot and cache
   * maintenance, then the RISAF/bookkeeping reads) are usually below one tick
   * of the system timer, which is why they are counted in DWT cycles.
   */

  stm32n6_aton_wd_puts("[perf] prep=");
  stm32n6_aton_wd_hex(g_aton_perf_c1 - g_aton_perf_c0);
  stm32n6_aton_wd_puts("cyc npu=");
  stm32n6_aton_wd_hex(g_aton_perf_c2 - g_aton_perf_c1);
  stm32n6_aton_wd_puts("cyc/");
  stm32n6_aton_wd_u32(m->last_usec);
  stm32n6_aton_wd_puts("us post=");
  stm32n6_aton_wd_hex(g_aton_perf_c3 - g_aton_perf_c2);
  stm32n6_aton_wd_puts("cyc\r\n");

  priv->events = aton_model_irq_count();
  m->runs      = aton_model_runs(id);

  aton_model_report("run finished");

  if (g_aton_perf_diag && g_aton_perf_runs++ == 0)
    {
      /* This run carried the trajectory dump and the per-run reports.  From
       * the next one on they are suppressed, so what the application prints is
       * the inference itself - that is the number the performance work needs.
       */

      g_aton_perf_diag = false;
      aton_model_set_verbose(false);
      stm32n6_aton_wd_puts("[perf] per-run diagnostics off from the next run"
                           "\r\n");
    }

  if (ret < 0)
    {
      aton_model_dump_log();
      stm32n6_aton_rif_fault_dump();
    }

  params->result = ret;
  params->usec   = m->last_usec;
  params->runs   = m->runs;
  params->events = priv->events;

  m->last_result = ret;

  nxmutex_unlock(&priv->lock);

  return ret;
}

/****************************************************************************
 * Name: stm32n6_aton_aie_get_output
 *
 * Description:
 *   Report the result of the last inference (AIE_CMD_GET_OUTPUT).  The output
 *   tensor itself stays in the NPU pool: the application reads it at the
 *   address returned by STM32N6_ATON_CMD_GET_STATUS.
 *
 ****************************************************************************/

static int stm32n6_aton_aie_get_output(FAR struct aie_lowerhalf_s *lower,
                                       FAR void *context, uintptr_t output)
{
  FAR struct stm32n6_aton_lowerhalf_s *priv =
    (FAR struct stm32n6_aton_lowerhalf_s *)lower;
  FAR struct stm32n6_aton_invoke_params *params =
    (FAR struct stm32n6_aton_invoke_params *)(uintptr_t)output;
  FAR struct stm32n6_aton_model_s *m;
  int id;

  UNUSED(context);

  if (params == NULL)
    {
      return -EINVAL;
    }

  id = (int)params->model_id;

  if (id < 0 || id >= ATON_MODEL_NMODELS)
    {
      return -ENODEV;
    }

  m = &priv->model[id];

  params->result = m->last_result;
  params->usec   = m->last_usec;
  params->runs   = m->runs;
  params->events = priv->events;

  return m->last_result;
}

/****************************************************************************
 * Name: stm32n6_aton_aie_control
 *
 * Description:
 *   Driver specific commands: tensor sizes and the address of the output
 *   inside the NPU pool (STM32N6_ATON_CMD_GET_STATUS), and a raw string
 *   channel that does not depend on the console driver
 *   (STM32N6_ATON_CMD_PUTS).
 *
 ****************************************************************************/

static int stm32n6_aton_aie_control(FAR struct aie_lowerhalf_s *lower,
                                    FAR void *context, int cmd,
                                    unsigned long arg)
{
  FAR struct stm32n6_aton_lowerhalf_s *priv =
    (FAR struct stm32n6_aton_lowerhalf_s *)lower;
  FAR struct stm32n6_aton_model_s *m;
  int ret = -ENOSYS;

  UNUSED(context);

  switch (cmd)
    {
      case STM32N6_ATON_CMD_GET_STATUS:
        {
          FAR struct stm32n6_aton_status_s *status =
            (FAR struct stm32n6_aton_status_s *)arg;
          int id;

          if (status == NULL)
            {
              return -EINVAL;
            }

          /* model_id is an input field here, so it has to be read before the
           * rest of the block is cleared.
           */

          id = (int)status->model_id;

          if (id < 0 || id >= ATON_MODEL_NMODELS)
            {
              return -ENODEV;
            }

          m = &priv->model[id];

          memset(status, 0, sizeof(struct stm32n6_aton_status_s));

          status->model_id    = (uint32_t)id;
          status->name        = aton_model_name(id);

          /* The epoch program resolved its input address once, when it was
           * published; this is the only buffer whose content reaches the
           * engines, so hand it to the application instead of letting it
           * guess (a wrong buffer produces a result that never changes).
           */

          status->input_addr  = m->input_addr;
          status->input_size  = m->input_size;
          status->output_addr = m->output_addr;
          status->output_size = m->output_size;
          status->runs        = m->runs;
          status->last_usec   = m->last_usec;
          status->events      = priv->events;
          status->last_result = m->last_result;

          ret = OK;
        }
        break;

      case STM32N6_ATON_CMD_PUTS:
        {
          FAR const char *str = (FAR const char *)(uintptr_t)arg;

          if (str == NULL)
            {
              return -EINVAL;
            }

          /* Polled output: this exists so a diagnostic can be printed even
           * when the console driver (and its TX semaphore) is what stalled.
           */

          while (*str != '\0')
            {
              up_putc((int)(unsigned char)*str++);
            }

          ret = OK;
        }
        break;

      default:
        break;
    }

  return ret;
}

/****************************************************************************
 * Name: stm32n6_aton_readiness_probe
 *
 * Description:
 *   Print what the NPU hard blocks say about themselves (their VERSION
 *   registers) and whether the NPU cache RAM is enabled.  These are the
 *   values that make "the device is there but refuses to work" visible: a
 *   block version of zero means the block is not clocked, not released from
 *   reset, or not reachable at all.
 *
 ****************************************************************************/

static void stm32n6_aton_readiness_probe(void)
{
  uint32_t clkctrl = getreg32(STM32_ATON_BLK_CLKCTRL + 0x04);
  uint32_t busif   = getreg32(STM32_ATON_BLK_BUSIF0 + 0x04);
  uint32_t intctrl = getreg32(STM32_ATON_BLK_INTCTRL + 0x04);
  uint32_t streng  = getreg32(STM32_ATON_BLK_STRENG + 0x04);
  uint32_t convacc = getreg32(STM32_ATON_BLK_CONVACC + 0x04);
  uint32_t decun   = getreg32(STM32_ATON_BLK_DECUN + 0x04);
  uint32_t activ   = getreg32(STM32_ATON_BLK_ACTIV + 0x04);
  uint32_t arith   = getreg32(STM32_ATON_BLK_ARITH + 0x04);
  uint32_t pool    = getreg32(STM32_ATON_BLK_POOL + 0x04);

  _info("ATON blocks: clkctrl=0x%08lx busif=0x%08lx intctrl=0x%08lx "
        "streng=0x%08lx convacc=0x%08lx decun=0x%08lx activ=0x%08lx "
        "arith=0x%08lx pool=0x%08lx\n", (unsigned long)clkctrl,
        (unsigned long)busif, (unsigned long)intctrl, (unsigned long)streng,
        (unsigned long)convacc, (unsigned long)decun, (unsigned long)activ,
        (unsigned long)arith, (unsigned long)pool);

  _info("ATON npu cache: enabled=%d sr=0x%08lx\n",
        stm32n6_cacheaxi_is_enabled() ? 1 : 0,
        (unsigned long)stm32n6_cacheaxi_status());
}

/****************************************************************************
 * Name: stm32n6_aton_runtime_selftest
 *
 * Description:
 *   Bring the ST Edge AI runtime up and report the versions it was built
 *   from.  This runs once, from the driver's initialisation, so a mismatched
 *   runtime (or a runtime that refuses to start) is reported before the first
 *   inference is attempted.
 *
 ****************************************************************************/

static int stm32n6_aton_runtime_selftest(void)
{
  stai_runtime_info info;
  stai_return_code  status;

  stm32n6_aton_readiness_probe();

  status = stai_runtime_init();
  if (status != STAI_SUCCESS)
    {
      _err("ATON: stai_runtime_init() failed: 0x%06lx\n",
           (unsigned long)status);
      return -EIO;
    }

  memset(&info, 0, sizeof(info));

  status = stai_runtime_get_info(&info);
  if (status != STAI_SUCCESS)
    {
      _err("ATON: stai_runtime_get_info() failed: 0x%06lx\n",
           (unsigned long)status);
      return -EIO;
    }

  _info("ATON runtime up: api %u.%u.%u runtime %u.%u.%u tools %u.%u.%u "
        "build 0x%08lx compiler %s\n", info.api_version.major,
        info.api_version.minor, info.api_version.micro,
        info.runtime_version.major, info.runtime_version.minor,
        info.runtime_version.micro, info.tools_version.major,
        info.tools_version.minor, info.tools_version.micro,
        (unsigned long)info.runtime_build, info.compiler_desc);

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_aton_aie_initialize
 *
 * Description:
 *   Bring the ATON accelerator to a state where a session can be started:
 *   release the NPU (and its cache RAM) from reset, open the RIF windows it
 *   needs, give it a coherent view of the non-secure memory, install the
 *   interrupt lines and register the character device.
 *
 *   The order matters and is not the one the reference manual suggests:
 *   the interrupt lines are attached before the driver is registered, so the
 *   NPU can never raise an interrupt into an unhandled vector, and the secure
 *   attributes of the NPU master (RIMC_ATTR1) are set before the first epoch
 *   program is published.
 *
 ****************************************************************************/


/****************************************************************************
 * Name: stm32n6_aton_ic6_boost
 *
 * Description:
 *   r76 experiment: raise IC6 -- the SYSCLK "domain B" divider that feeds
 *   the NPU -- from the FSBL's PLL1/4 = 300 MHz to PLL1/2 = 600 MHz.
 *
 *   Evidence: ST's own 995_AI_Hand_Landmarks application reconfigures IC6
 *   right before it enables the NPU clock (Appli/Core/Src/main.c:
 *   IC6SEL = PLL2, IC6DIV = 1 => 1000 MHz, then
 *   __HAL_RCC_NPU_CLK_ENABLE()), i.e. the NPU hangs off IC6.  The
 *   ALIENTEK FSBL chain-loads NuttX with PLL2 off and IC6 = PLL1/4 =
 *   300 MHz, so the NPU would run at roughly a third of ST's clock.
 *
 *   Only the INT divider field changes; PLL1 is already running at
 *   1200 MHz (FSBL M=4 / N=75), so no PLL is started or stopped.  This is
 *   deliberately a single-variable experiment: if the epoch cycle count
 *   does not drop, the NPU is not clocked by IC6.
 *
 *   IC6 feeds SYSCLK "domain B" only.  SYSCLK itself comes from IC2
 *   (PLL1/3 = 400 MHz) and every kernel clock in use on this board is
 *   routed elsewhere (USART1 = HSI, XSPI2 = IC3, SDMMC = IC4, LTDC =
 *   IC16, DCMIPP = IC17, CSI = IC18), so console/NOR/display are not
 *   affected.
 *
 *   The divider must not be changed while it is enabled: clear DIVEN,
 *   write the new ratio, then re-enable.  This port defines no DIVENCR
 *   alias, so DIVEN is cleared with a read-modify-write on DIVENR.
 *
 ****************************************************************************/

/* r78: back to the measured optimum.  r76 (IC6 = PLL1/2 = 600 MHz) took the
 * epoch loop from 259.8 M to 146.1 M cycles; r77 (IC6 = PLL2/1, nominally
 * 1000 MHz) configured *correctly* -- PLL2CFGR1 read back as M=8/N=125,
 * PLL2RDY = 1, IC6CFGR = 0x10000000 -- yet measured 194.4 M, which the
 * A/f + B model (A = 68262 Mcycle*MHz, B = 32.3 Mcycle, fitted from the
 * 300/600 MHz points) converts to an *effective* 421 MHz.
 *
 * Crossing the NPU domain to a second PLL therefore costs more than the
 * frequency buys back, so IC6 stays on PLL1 -- the same source as IC2/SYSCLK.
 * (The three fill checksums are identical across r76 and r77, so neither
 * configuration perturbs the arithmetic; only the speed differs.)
 *
 * To retry 1200 MHz on the same PLL -- above the NPU's 1 GHz rating, an
 * experiment only -- set STM32N6_ATON_IC6_DIV to 1 and rebuild.
 */

#define STM32N6_ATON_IC6_DIV 2 /* PLL1 / 2 = 600 MHz (measured optimum) */

static void stm32n6_aton_ic6_boost(void)
{
  uint32_t before = getreg32(STM32_RCC_IC6CFGR);
  uint32_t denr   = getreg32(STM32_RCC_DIVENR);

  modifyreg32(STM32_RCC_DIVENR, RCC_DIVENR_IC6EN, 0);
  putreg32(RCC_ICCFGR_SEL_PLL1 |
           ((STM32N6_ATON_IC6_DIV - 1) << RCC_ICCFGR_INT_SHIFT),
           STM32_RCC_IC6CFGR);
  putreg32(RCC_DIVENR_IC6EN, STM32_RCC_DIVENSR);

  _info("IC6CFGR %08lx -> %08lx (PLL1/4 = 300 MHz -> PLL1/%u = 600 MHz)\n",
        (unsigned long)before, (unsigned long)getreg32(STM32_RCC_IC6CFGR),
        (unsigned)STM32N6_ATON_IC6_DIV);
  _info("DIVENR %08lx -> %08lx (IC6EN bit %lu)\n",
        (unsigned long)denr, (unsigned long)getreg32(STM32_RCC_DIVENR),
        (unsigned long)((getreg32(STM32_RCC_DIVENR) >> 5) & 1u));
}

int stm32n6_aton_aie_initialize(void)
{
  FAR struct stm32n6_aton_lowerhalf_s *priv;
  int ret;
  int i;

  stm32n6_aton_wd_puts("[aton] diag build r78 (IC6 back on PLL1/2 = 600 MHz: "
                       "PLL2/1 was configured correctly but measured 1.33x "
                       "SLOWER, so the NPU domain stays on PLL1)\r\n");

  priv = (FAR struct stm32n6_aton_lowerhalf_s *)
         zalloc(sizeof(struct stm32n6_aton_lowerhalf_s));
  if (priv == NULL)
    {
      _err("ERROR: Failed to allocate STM32N6 ATON lower-half\n");
      return -ENOMEM;
    }

  priv->lower.ops = &g_stm32n6_aton_aie_ops;
  nxmutex_init(&priv->lock);

  /* r76: raise the NPU clock-domain divider before the NPU gate opens.
   * See stm32n6_aton_ic6_boost() for the evidence and the risk analysis.
   */

  stm32n6_aton_ic6_boost();

  /* Clock the NPU and its cache RAM, then pulse their resets: the RM lists
   * the reset as the last step of the sequence, and the blocks come up with
   * a zero VERSION register when only the clock (or only the reset) is done.
   */

  modifyreg32(STM32_RCC_AHB5ENR, 0, RCC_AHB5ENR_NPUEN);
  modifyreg32(STM32_RCC_AHB5RSTR, 0, RCC_AHB5RSTR_NPURST);
  modifyreg32(STM32_RCC_AHB5RSTR, RCC_AHB5RSTR_NPURST, 0);

  modifyreg32(STM32_RCC_AHB5ENR, 0, RCC_AHB5ENR_CACHEAXIEN);
  modifyreg32(STM32_RCC_AHB5RSTR, 0, RCC_AHB5RSTR_CACHEAXIRST);
  modifyreg32(STM32_RCC_AHB5RSTR, RCC_AHB5RSTR_CACHEAXIRST, 0);

  /* Open the windows the NPU needs, before the CPU tries to touch any of the
   * memory it shares with the accelerator.
   */

  stm32n6_aton_risaf_clear();
  stm32n6_aton_risaf_map_dump("before");

  stm32n6_aton_ns_setup();

  stm32n6_aton_risaf_nor_open();
  stm32n6_aton_risaf_map_dump("after");

  /* The NPU cache RAM is not a cache in the CPU sense: it is a scratchpad the
   * runtime uses for its tensor accesses, and it has to be initialized and
   * enabled before any inference.
   */

  npu_cache_init();
  npu_cache_enable();

  /* RIMC_ATTR1: the ATON master attributes (maskable security attribute
   * register).  0x310 is MSEC (bit 8) | MPRIV (bit 9) | MCID 1 (bits 6:4),
   * i.e. "secure, privileged, CID 1" - the default domain of every RIF window
   * this driver opens, and the only one that may read the weights in the NOR
   * flash.
   */

  putreg32(0x00000310u, STM32N6_ATON_RIMC_ATTR1); /* NB: value first */

  stm32n6_aton_wd_puts("[rimc] npu attr=");
  stm32n6_aton_wd_hex(getreg32(STM32N6_ATON_RIMC_ATTR1));
  stm32n6_aton_wd_puts(" (want CID=1 secure priv)\r\n");

  /* Attach and enable the four NPU interrupt lines.  The driver's handler
   * stays installed until the runtime installs its own for line 0; keeping
   * the lines enabled from here on means a stray NPU interrupt is counted
   * rather than reported as an unexpected interrupt.
   */

  for (i = 0; i < (int)STM32N6_ATON_NIRQS; i++)
    {
      ret = irq_attach(g_stm32n6_aton_npu_irqs[i], stm32n6_aton_npu_irq,
                       priv);
      if (ret < 0)
        {
          _err("ERROR: Failed to attach NPU IRQ %d: %d\n",
               g_stm32n6_aton_npu_irqs[i], ret);

          while (i > 0)
            {
              i--;
              up_disable_irq(g_stm32n6_aton_npu_irqs[i]);
              irq_attach(g_stm32n6_aton_npu_irqs[i], NULL, NULL);
            }

          nxmutex_destroy(&priv->lock);
          free(priv);
          return ret;
        }

      up_enable_irq(g_stm32n6_aton_npu_irqs[i]);
    }

  ret = aie_register(CONFIG_STM32N6_ATON_AIE_DEVPATH, &priv->lower);
  if (ret < 0)
    {
      _err("ERROR: Failed to register STM32N6 ATON AIE driver: %d\n", ret);

      for (i = 0; i < (int)STM32N6_ATON_NIRQS; i++)
        {
          up_disable_irq(g_stm32n6_aton_npu_irqs[i]);
          irq_attach(g_stm32n6_aton_npu_irqs[i], NULL, NULL);
        }

      nxmutex_destroy(&priv->lock);
      free(priv);
      return ret;
    }

  _info("STM32N6 ATON AIE driver registered at %s\n",
        CONFIG_STM32N6_ATON_AIE_DEVPATH);

  /* Report the state of the accelerator the application is about to use:
   * block versions, the NPU cache and the runtime versions.
   */

  stm32n6_aton_runtime_selftest();

  return OK;
}
