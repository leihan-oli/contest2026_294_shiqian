/****************************************************************************
 * libs/ai_aton/ll_aton_osal_nuttx.c
 *
 * NuttX implementation of the OSAL hooks required by the ST Neural-ART
 * (ATON) runtime.  See ll_aton_osal_user_impl.h for the macro layer and
 * the middleware's ll_aton_osal_freertos.c for the reference semantics.
 *
 * Mapping onto NuttX primitives:
 *
 *   LL_ATON_OSAL_WFE            nxsem_wait() on a binary semaphore that the
 *                               NPU interrupt handler posts
 *   LL_ATON_OSAL_SIGNAL_EVENT   nxsem_post() from that handler
 *   LL_ATON_LOCK/UNLOCK_ATON    nxmutex:  serializes epochs and, unlike the
 *                               FreeRTOS port's hand-made "priority based"
 *                               scheme, gets priority inheritance for free
 *   cache locks                 nxmutex around cache maintenance
 *   ENTER/EXIT_CS               mask/unmask the ATON interrupt line
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <errno.h>
#include <debug.h>

#include <nuttx/arch.h>
#include <nuttx/irq.h>
#include <nuttx/mutex.h>
#include <nuttx/panic_notifier.h>
#include <nuttx/semaphore.h>

#include "ll_aton_osal_user_impl.h"

/****************************************************************************
 * ATON clock gates
 *
 * The vendor runtime gates every accelerator clock when it is built with
 * LL_ATON_ENABLE_CLOCK_GATING=1: ll_aton.c writes BGATES=0 and only then
 * calls this OSAL hook.  A model that needs a convolution, pooling or
 * decompression unit afterwards stops on that opcode and reports nothing at
 * all - no error interrupt, no opcode counter change, and the unit's CTRL
 * register reads back 0x00000000 because it never sees the configuration
 * write.
 *
 * Opening the gates here - right after the runtime closed them - is exactly
 * what the same runtime does when clock gating is disabled.
 */

#define ATON_OSAL_NPU_BASE        0x580e0000u
#define ATON_OSAL_CLKCTRL_CTRL    (ATON_OSAL_NPU_BASE + 0x00)
#define ATON_OSAL_CLKCTRL_AGATES0 (ATON_OSAL_NPU_BASE + 0x08)
#define ATON_OSAL_CLKCTRL_AGATES1 (ATON_OSAL_NPU_BASE + 0x0c)
#define ATON_OSAL_CLKCTRL_BGATES  (ATON_OSAL_NPU_BASE + 0x10)

#ifdef CONFIG_LIB_AI_ATON

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int aton_osal_nuttx_panic(FAR struct notifier_block *nb,
                                 unsigned long action, FAR void *data);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Posted by the NPU interrupt handler, waited on by the ATON runtime.  A
 * binary semaphore started at zero, exactly like the FreeRTOS port's
 * _wfe_sem.
 */

static sem_t g_aton_wfe_sem;

/* Number of epoch events signalled by the NPU interrupt handler (diagnostics;
 * one inference may need several events).
 */

static volatile uint32_t g_aton_event_count;

/* ISR-safe log ring.  The ATON runtime reports epoch errors from interrupt
 * context, where printf() is not usable on NuttX (the console takes a mutex -
 * that is what made the first NPU run panic in nxmutex_wait()).  vsnprintf()
 * into a static buffer needs neither locks nor allocation, so it is safe from
 * an ISR; the lines are dumped from task context with
 * aton_osal_nuttx_log_dump().
 */

/* Keep enough lines that the first events of a run survive behind a burst of
 * repeated ones (an error storm used to push the interesting entries out).
 */

#define ATON_OSAL_LOG_LINES   64
#define ATON_OSAL_LOG_LINELEN 160

static char g_aton_log[ATON_OSAL_LOG_LINES][ATON_OSAL_LOG_LINELEN];
static volatile unsigned int g_aton_log_head;   /* next slot to fill */
static volatile unsigned int g_aton_log_total;  /* lines ever recorded */

/* Epoch loop watchdog: LL_ATON_OSAL_WFE() waits with a timeout so that a lost
 * or errored epoch cannot block the caller forever.
 */

#define ATON_OSAL_WFE_TIMEOUT_MS  50
#define ATON_OSAL_WFE_MAX_TIMEOUT 40    /* 40 x 50 ms = 2 s of no progress */
#define ATON_OSAL_WFE_STALLED_MS  1000  /* slow poll once the stall is known */

static volatile unsigned int g_aton_wfe_timeouts;
static volatile bool         g_aton_wfe_stalled;

/* Serializes ATON epochs (LL_ATON_LOCK_ATON/UNLOCK_ATON). */

static mutex_t g_aton_epoch_mutex;

/* Serializes cache maintenance on both the MCU and the NPU cache
 * (LL_ATON_LOCK_MCU_CACHE / LL_ATON_LOCK_NPU_CACHE).
 */

static mutex_t g_aton_cache_mutex;

static bool g_aton_osal_initialized;

/* Interrupt accounting and storm protection.
 *
 * If the NPU keeps its interrupt line asserted (an error condition that comes
 * back right after the status was cleared), the CPU never returns to thread
 * mode: the ISR completes and the NVIC takes the very same interrupt again.
 * No task then makes progress - not even a timed wait can expire - so an
 * inference looks like a silent hang.
 *
 * Counting the interrupts per time window detects that and masks the line,
 * which hands the CPU back to the runtime so that its watchdog can report the
 * stall and dump the recorded error details.
 */

static volatile uint32_t g_aton_irq_count;      /* total NPU interrupts */
static volatile uint32_t g_aton_irq_window;     /* tick this window started */
static volatile uint32_t g_aton_irq_in_window;  /* interrupts in this window */
static volatile bool     g_aton_irq_storm;      /* storm -> line masked */
static volatile int      g_aton_stage;          /* last OSAL hook entered */
static volatile uint32_t g_aton_wfe_count;      /* WFE waits entered */

/* Storm thresholds: more than this many interrupts inside the window means the
 * line is not making progress, it is just being re-asserted.
 */

#define ATON_OSAL_IRQ_STORM_WINDOW MSEC2TICK(100)
#define ATON_OSAL_IRQ_STORM_LIMIT  200

/* Panic hook: the runtime records whatever the epoch controller reported into
 * the ring log, but that transcript is only printed when the runtime returns
 * control.  If the board dies first (fault, assertion, watchdog) the evidence
 * of *why* the NPU stopped would be lost - so the ring is dumped from the
 * panic notifier chain as well.
 */

static struct notifier_block g_aton_panic_nb =
{
  .notifier_call = aton_osal_nuttx_panic,
  .priority      = 0,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: aton_stage / aton_stage_name
 *
 * Description:
 *   Remember the last OSAL hook the runtime entered, so a stuck inference can
 *   be attributed to a phase (waiting for the NPU vs. doing cache work vs.
 *   holding a lock) when the watchdog reports it.
 *
 ****************************************************************************/

enum aton_stage_e
{
  ATON_STAGE_IDLE = 0,
  ATON_STAGE_INIT,
  ATON_STAGE_IRQ_INSTALL,
  ATON_STAGE_IRQ_ENABLE,
  ATON_STAGE_IRQ_DISABLE,
  ATON_STAGE_WFE,
  ATON_STAGE_WFE_RETURN,
  ATON_STAGE_NPU_ISR,
  ATON_STAGE_CS_ENTER,
  ATON_STAGE_CS_EXIT,
  ATON_STAGE_LOCK,
  ATON_STAGE_UNLOCK,
  ATON_STAGE_CACHE_LOCK,
  ATON_STAGE_CACHE_UNLOCK,
  ATON_STAGE_MAX
};

static FAR const char *aton_stage_name(int stage)
{
  switch (stage)
    {
      case ATON_STAGE_INIT:        return "osal-init";
      case ATON_STAGE_IRQ_INSTALL: return "irq-install";
      case ATON_STAGE_IRQ_ENABLE:  return "irq-enable";
      case ATON_STAGE_IRQ_DISABLE: return "irq-disable";
      case ATON_STAGE_WFE:         return "waiting-for-NPU (wfe)";
      case ATON_STAGE_WFE_RETURN:  return "wfe-returned";
      case ATON_STAGE_NPU_ISR:     return "in-NPU-ISR";
      case ATON_STAGE_CS_ENTER:    return "critical-section";
      case ATON_STAGE_CS_EXIT:     return "critical-section-left";
      case ATON_STAGE_LOCK:        return "aton-lock";
      case ATON_STAGE_UNLOCK:      return "aton-unlock";
      case ATON_STAGE_CACHE_LOCK:  return "cache-lock";
      case ATON_STAGE_CACHE_UNLOCK:return "cache-unlock";
      default:                     return "idle";
    }
}

/****************************************************************************
 * Name: aton_osal_nuttx_init
 *
 * Description:
 *   Create the OS objects used by the runtime.  Called once, from
 *   LL_ATON_OSAL_INIT() (stai_runtime_init()).
 *
 ****************************************************************************/


/* ------------------------------------------------------------------------
 * r31: never-sleep epoch wait + dataflow trajectory probe
 *
 * Every round so far ends the same way: the epoch controller starts the
 * dataflow, the two stream engines move a few kilobytes (a different amount
 * on every boot) and both wedge in the middle of the transfer, with zero
 * errors anywhere.  The one global event inside that window is the CPU
 * entering CSLEEP: this wait blocks in nxsem_tickwait(), so the idle task
 * runs up_idle()/WFI microseconds after the epoch start.
 *
 * This probe therefore (a) never sleeps while a wait is in flight and
 * (b) records the trajectory of the epoch (CPU cycle stamps plus the stream
 * engines' progress counters), so the freeze instant and the throughput are
 * measured instead of guessed.
 * ------------------------------------------------------------------------ */

#define ATON_OSAL_NOSLEEP        1
#define ATON_OSAL_NPU_BASE       0x580e0000u
#define ATON_OSAL_EC_BC_REG      (ATON_OSAL_NPU_BASE + 0x1e020u)
#define ATON_OSAL_PIXCNT_OFF     0x60u
#define ATON_OSAL_STR3_REG       (ATON_OSAL_NPU_BASE + 0x5000u + 0x3000u)
#define ATON_OSAL_STR9_REG       (ATON_OSAL_NPU_BASE + 0x5000u + 0x9000u)
#define ATON_OSAL_DEMCR          0xe000edfcu
#define ATON_OSAL_DWT_CTRL       0xe0001000u
#define ATON_OSAL_DWT_CYCCNT     0xe0001004u
#define ATON_TRAJ_MAX            48
#define ATON_TRAJ_GAP_CYC        6000u

#define ATON_RD32(a)             (*(volatile uint32_t *)(uintptr_t)(a))
#define ATON_WR32(a, v)          (*(volatile uint32_t *)(uintptr_t)(a) = (v))

static volatile uint32_t g_aton_traj_cyc[ATON_TRAJ_MAX];
static volatile uint32_t g_aton_traj_bc[ATON_TRAJ_MAX];
static volatile uint32_t g_aton_traj_p3[ATON_TRAJ_MAX];
static volatile uint32_t g_aton_traj_p9[ATON_TRAJ_MAX];
static volatile uint32_t g_aton_traj_n;
static volatile uint32_t g_aton_traj_changes;
static volatile uint32_t g_aton_traj_t0;
static volatile uint32_t g_aton_traj_stored;
static volatile uint32_t g_aton_traj_frz;
static volatile uint32_t g_aton_traj_p3last;
static volatile uint32_t g_aton_traj_end;
static volatile int      g_aton_traj_state;
static volatile bool     g_aton_traj_reported;

static void aton_traj_str(FAR const char *s)
{
  while (*s != '\0')
    {
      up_putc(*s++);
    }
}

static void aton_traj_hex(uint32_t v)
{
  int i;

  for (i = 28; i >= 0; i -= 4)
    {
      up_putc("0123456789abcdef"[(v >> i) & 0xf]);
    }
}

static void aton_traj_kv(FAR const char *k, uint32_t v)
{
  aton_traj_str(" ");
  aton_traj_str(k);
  aton_traj_str("=");
  aton_traj_hex(v);
}

static void aton_traj_report(FAR const char *tag)
{
  uint32_t i;

  aton_traj_str("\r\n[tj] tag=");
  aton_traj_str(tag);
  aton_traj_kv("t0", g_aton_traj_t0);
  aton_traj_kv("end", g_aton_traj_end - g_aton_traj_t0);
  aton_traj_kv("frz", g_aton_traj_frz - g_aton_traj_t0);
  aton_traj_kv("chg", g_aton_traj_changes);
  aton_traj_kv("n", g_aton_traj_n);
  aton_traj_kv("bc", ATON_RD32(ATON_OSAL_EC_BC_REG));
  aton_traj_kv("p3", ATON_RD32(ATON_OSAL_STR3_REG + ATON_OSAL_PIXCNT_OFF));
  aton_traj_kv("p9", ATON_RD32(ATON_OSAL_STR9_REG + ATON_OSAL_PIXCNT_OFF));
  aton_traj_kv("tick", clock_systime_ticks());

  for (i = 0; i < g_aton_traj_n && i < 24; i++)
    {
      aton_traj_str("\r\n[tj]  i=");
      aton_traj_hex(i);
      aton_traj_kv("dc", g_aton_traj_cyc[i] - g_aton_traj_t0);
      aton_traj_kv("bc", g_aton_traj_bc[i]);
      aton_traj_kv("p3", g_aton_traj_p3[i]);
      aton_traj_kv("p9", g_aton_traj_p9[i]);
    }

  aton_traj_str("\r\n");
}

/* r73: the trajectory dump and the per-run report are a first-run diagnostic.
 * Their ~25 UART lines sit *inside* the timed inference window (about 150 ms at
 * 115200 baud, i.e. more than the NPU itself takes), so the driver turns them
 * off once the first inference has been traced and the times printed for the
 * later runs are the real ones.
 */

static volatile bool g_aton_osal_verbose = true;

void aton_osal_nuttx_set_verbose(bool verbose)
{
  g_aton_osal_verbose = verbose;
}

static void aton_traj_poll(void)
{
  uint32_t cyc;
  uint32_t bc;
  uint32_t p3;
  uint32_t p9;

  if (!g_aton_osal_verbose)
    {
      return;
    }

  cyc = ATON_RD32(ATON_OSAL_DWT_CYCCNT);
  bc  = ATON_RD32(ATON_OSAL_EC_BC_REG);
  p3  = ATON_RD32(ATON_OSAL_STR3_REG + ATON_OSAL_PIXCNT_OFF);
  p9  = ATON_RD32(ATON_OSAL_STR9_REG + ATON_OSAL_PIXCNT_OFF);

  if (g_aton_traj_state == 0)
    {
      g_aton_traj_state   = 1;
      g_aton_traj_t0      = cyc;
      g_aton_traj_frz     = cyc;
      g_aton_traj_stored  = cyc - ATON_TRAJ_GAP_CYC;
      g_aton_traj_p3last  = p3;
      g_aton_traj_changes = 0;
      g_aton_traj_n       = 0;

      aton_traj_str("\r\n[tj] start bc=");
      aton_traj_hex(bc);
      aton_traj_kv("p3", p3);
      aton_traj_kv("p9", p9);
      aton_traj_kv("cyc", cyc);
    }

  if (p3 != g_aton_traj_p3last)
    {
      g_aton_traj_changes++;
      g_aton_traj_frz    = cyc;
      g_aton_traj_p3last = p3;
    }

  if (g_aton_traj_n < ATON_TRAJ_MAX &&
      (uint32_t)(cyc - g_aton_traj_stored) >= ATON_TRAJ_GAP_CYC)
    {
      g_aton_traj_stored             = cyc;
      g_aton_traj_cyc[g_aton_traj_n] = cyc;
      g_aton_traj_bc[g_aton_traj_n]  = bc;
      g_aton_traj_p3[g_aton_traj_n]  = p3;
      g_aton_traj_p9[g_aton_traj_n]  = p9;
      g_aton_traj_n++;
    }
}

static int aton_osal_nuttx_poll_wait(uint32_t timeout_ticks)
{
  uint32_t tstart = clock_systime_ticks();
  int      ret;

  for (;;)
    {
      ret = nxsem_trywait(&g_aton_wfe_sem);
      if (ret == OK)
        {
          return OK;
        }

      aton_traj_poll();

      if ((uint32_t)(clock_systime_ticks() - tstart) >= timeout_ticks)
        {
          return -ETIMEDOUT;
        }
    }
}

void aton_osal_nuttx_init(void)
{
  int ret;

  if (g_aton_osal_initialized)
    {
      return;
    }

  ret = nxsem_init(&g_aton_wfe_sem, 0, 0);
  if (ret < 0)
    {
      _err("ERROR: nxsem_init(wfe) failed: %d\n", ret);
      return;
    }

  nxmutex_init(&g_aton_epoch_mutex);
  nxmutex_init(&g_aton_cache_mutex);

  g_aton_log_head      = 0;
  g_aton_log_total     = 0;
  g_aton_wfe_timeouts  = 0;
  g_aton_wfe_stalled   = false;
  /* r31: free-running CPU cycle counter as the trajectory time base.  It
   * only advances while the core actually executes, which also makes the
   * CSLEEP windows visible in the trace.
   */

  ATON_WR32(ATON_OSAL_DEMCR, ATON_RD32(ATON_OSAL_DEMCR) | (1u << 24));
  ATON_WR32(ATON_OSAL_DWT_CTRL, ATON_RD32(ATON_OSAL_DWT_CTRL) | 1u);
  g_aton_event_count   = 0;

  g_aton_osal_initialized = true;
  g_aton_irq_count        = 0;
  g_aton_irq_window       = clock_systime_ticks();
  g_aton_irq_in_window    = 0;
  g_aton_irq_storm        = false;
  g_aton_stage            = ATON_STAGE_INIT;

  /* Open every ATON clock gate before the model runs.  Written through a
   * volatile pointer: this file has no putreg32() available.
   */

  {
    volatile uint32_t *clk = (volatile uint32_t *)ATON_OSAL_NPU_BASE;

    clk[0x00 / 4] = 1;               /* CLKCTRL_CTRL   : enable the unit  */
    clk[0x08 / 4] = 0xffffffff;      /* CLKCTRL_AGATES0: all A gates     */
    clk[0x0c / 4] = 0xffffffff;      /* CLKCTRL_AGATES1: all A gates     */
    clk[0x10 / 4] = 0xffffffff;      /* CLKCTRL_BGATES : all B gates     */
  }

  {
    FAR const char *msg = "[aton] npu clock gates opened\r\n";
    while (*msg != '\0')
      {
        up_putc((int)(unsigned char)*msg++);
      }
  }

  panic_notifier_chain_register(&g_aton_panic_nb);

  _info("ATON OSAL ready (NPU IRQ line %d = NVIC %d)\n",
        ATON_STD_IRQ_LINE, ATON_OSAL_NUTTX_IRQ(ATON_STD_IRQ_LINE));
}

/****************************************************************************
 * Name: aton_osal_nuttx_deinit
 ****************************************************************************/

void aton_osal_nuttx_deinit(void)
{
  if (!g_aton_osal_initialized)
    {
      return;
    }

  nxmutex_destroy(&g_aton_cache_mutex);
  nxmutex_destroy(&g_aton_epoch_mutex);
  nxsem_destroy(&g_aton_wfe_sem);

  g_aton_osal_initialized = false;
}

/****************************************************************************
 * Name: aton_osal_nuttx_wfe
 *
 * Description:
 *   Block until the NPU signals epoch completion.  Never returns early: the
 *   runtime relies on this being an indefinite wait.
 *
 ****************************************************************************/

void aton_osal_nuttx_wfe(void)
{
  uint32_t timeout;
  int      ret;

  /* Bounded wait: an epoch that never completes (unexpected error interrupt,
   * lost event) must not turn into a silent hang.  After
   * ATON_OSAL_WFE_MAX_TIMEOUT consecutive timeouts the runtime's ISR log is
   * dumped here - this runs in the calling task's context, so printing is
   * legal - the NPU interrupt line is masked and the wait switches to a slow
   * poll so the CPU stays free for the shell.
   */

  timeout = g_aton_wfe_stalled ? MSEC2TICK(ATON_OSAL_WFE_STALLED_MS)
                               : MSEC2TICK(ATON_OSAL_WFE_TIMEOUT_MS);

  g_aton_wfe_count++;
  g_aton_stage = ATON_STAGE_WFE;

#if ATON_OSAL_NOSLEEP
  ret = aton_osal_nuttx_poll_wait(timeout);
#else
  ret = nxsem_tickwait(&g_aton_wfe_sem, timeout);
#endif

  g_aton_traj_end = ATON_RD32(ATON_OSAL_DWT_CYCCNT);

  if (ret == OK && !g_aton_traj_reported && g_aton_traj_state == 1)
    {
      g_aton_traj_reported = true;
      g_aton_traj_state    = 2;
      aton_traj_report("evt");
    }

  g_aton_stage = ATON_STAGE_WFE_RETURN;

  if (ret == OK)
    {
      g_aton_wfe_timeouts = 0;
      return;
    }

  if (ret == -ETIMEDOUT)
    {
      g_aton_wfe_timeouts++;

      if (!g_aton_wfe_stalled &&
          g_aton_wfe_timeouts >= ATON_OSAL_WFE_MAX_TIMEOUT)
        {
          g_aton_wfe_stalled = true;
          if (!g_aton_traj_reported)
            {
              g_aton_traj_reported = true;
              aton_traj_report("stall");
            }

          up_disable_irq(ATON_OSAL_NUTTX_IRQ(ATON_STD_IRQ_LINE));

          _err("ERROR: ATON epoch loop stalled (%u ms without progress); "
               "NPU interrupt masked, inference will not complete\n",
               ATON_OSAL_WFE_TIMEOUT_MS * ATON_OSAL_WFE_MAX_TIMEOUT);

          /* Show what the runtime recorded (epoch controller error details,
           * interrupt masks, ...).  This is task context.
           */

          aton_osal_nuttx_log_dump();
        }

      return;
    }

  aton_osal_nuttx_log("ATON: WFE interrupted (%d)\n", ret);
}

/****************************************************************************
 * Name: aton_osal_nuttx_log / aton_osal_nuttx_log_puts
 *
 * Description:
 *   Lock-free, ISR-safe logging used by the ATON runtime (its error paths run
 *   in interrupt context).  Lines are kept in a fixed ring and dumped later
 *   from task context.
 *
 ****************************************************************************/

/****************************************************************************
 * Name: aton_log_emit / aton_log_str / aton_log_num
 *
 * Description:
 *   Minimal, bounded and float-free formatter for the ISR-side log.
 *
 *   vsnprintf() cannot be used here: with CONFIG_LIBC_FLOATINGPOINT the float
 *   conversions pull in newlib's float formatting, which calls libm (powf and
 *   friends) and needs far more stack than an interrupt context has - that is
 *   what overflowed the 8KB interrupt stack and escalated to a HardFault
 *   (powf frames on the IRQ stack, g_last_regs in R0).
 *
 *   Supported: %s %c %d %i %u %x %X %o %p %% with optional width/zero padding
 *   and length modifiers.  Float conversions are rendered as "<f>" - the ATON
 *   messages that matter (EC_IRQ, opcode counter, label) are all hex.
 *
 ****************************************************************************/

static void aton_log_emit(char *dst, size_t dstlen, size_t *pos, char c)
{
  if (*pos + 1 < dstlen)
    {
      dst[(*pos)++] = c;
      dst[*pos]     = '\0';
    }
}

static void aton_log_str(char *dst, size_t dstlen, size_t *pos,
                         const char *str)
{
  if (str == NULL)
    {
      str = "(null)";
    }

  while (*str != '\0')
    {
      aton_log_emit(dst, dstlen, pos, *str++);
    }
}

static void aton_log_num(char *dst, size_t dstlen, size_t *pos,
                         uint32_t value, int base, int upper,
                         int width, char pad)
{
  char tmp[16];
  int  n = 0;

  if (value == 0)
    {
      tmp[n++] = '0';
    }

  while (value != 0)
    {
      uint32_t d = value % (uint32_t)base;
      tmp[n++]   = (d < 10) ? (char)('0' + d)
                            : (char)((upper ? 'A' : 'a') + d - 10);
      value     /= (uint32_t)base;
    }

  while (n < width && n < (int)sizeof(tmp))
    {
      tmp[n++] = pad;
    }

  while (n-- > 0)
    {
      aton_log_emit(dst, dstlen, pos, tmp[n]);
    }
}

static void aton_log_format(char *dst, size_t dstlen, const char *fmt,
                            va_list ap)
{
  size_t pos = 0;

  if (dstlen == 0)
    {
      return;
    }

  dst[0] = '\0';

  while (*fmt != '\0')
    {
      int width = 0;
      char pad = ' ';

      if (*fmt != '%')
        {
          aton_log_emit(dst, dstlen, &pos, *fmt++);
          continue;
        }

      fmt++;                                  /* skip '%' */

      if (*fmt == '%')
        {
          aton_log_emit(dst, dstlen, &pos, *fmt++);
          continue;
        }

      /* Flags / width / length modifiers (best effort, no positional args) */

      if (*fmt == '0')
        {
          pad = '0';
          fmt++;
        }

      while (*fmt >= '0' && *fmt <= '9')
        {
          width = width * 10 + (*fmt++ - '0');
        }

      while (*fmt == 'l' || *fmt == 'h' || *fmt == 'z' || *fmt == 'j' ||
             *fmt == 't')
        {
          fmt++;
        }

      switch (*fmt)
        {
          case 's':
            aton_log_str(dst, dstlen, &pos,
                         va_arg(ap, const char *));
            break;

          case 'c':
            aton_log_emit(dst, dstlen, &pos, (char)va_arg(ap, int));
            break;

          case 'd':
          case 'i':
            {
              int32_t v = va_arg(ap, int32_t);

              if (v < 0)
                {
                  aton_log_emit(dst, dstlen, &pos, '-');
                  aton_log_num(dst, dstlen, &pos, (uint32_t)(-v), 10, 0,
                               width, pad);
                }
              else
                {
                  aton_log_num(dst, dstlen, &pos, (uint32_t)v, 10, 0,
                               width, pad);
                }
            }
            break;

          case 'u':
            aton_log_num(dst, dstlen, &pos, va_arg(ap, uint32_t), 10, 0,
                         width, pad);
            break;

          case 'x':
            aton_log_num(dst, dstlen, &pos, va_arg(ap, uint32_t), 16, 0,
                         width, pad);
            break;

          case 'X':
            aton_log_num(dst, dstlen, &pos, va_arg(ap, uint32_t), 16, 1,
                         width, pad);
            break;

          case 'o':
            aton_log_num(dst, dstlen, &pos, va_arg(ap, uint32_t), 8, 0,
                         width, pad);
            break;

          case 'p':
            aton_log_str(dst, dstlen, &pos, "0x");
            aton_log_num(dst, dstlen, &pos, (uint32_t)va_arg(ap, void *),
                         16, 0, 8, '0');
            break;

          /* Float conversions: consume the argument, do not format it (libm
           * would be pulled into interrupt context).
           */

          case 'f':
          case 'F':
          case 'e':
          case 'E':
          case 'g':
          case 'G':
          case 'a':
          case 'A':
            (void)va_arg(ap, double);
            aton_log_str(dst, dstlen, &pos, "<f>");
            break;

          case '\0':
            return;

          default:
            aton_log_emit(dst, dstlen, &pos, '%');
            aton_log_emit(dst, dstlen, &pos, *fmt);
            break;
        }

      if (*fmt != '\0')
        {
          fmt++;
        }
    }
}

void aton_osal_nuttx_log(const char *fmt, ...)
{
  unsigned int slot = g_aton_log_head % ATON_OSAL_LOG_LINES;
  va_list ap;

  va_start(ap, fmt);
  aton_log_format(g_aton_log[slot], ATON_OSAL_LOG_LINELEN, fmt, ap);
  va_end(ap);

  g_aton_log_head  = slot + 1;
  g_aton_log_total = g_aton_log_total + 1;
}

void aton_osal_nuttx_log_puts(const char *str)
{
  aton_osal_nuttx_log("%s", str);
}

/****************************************************************************
 * Name: aton_osal_nuttx_log_dump
 *
 * Description:
 *   Print the recorded log lines (task context only).  The newest
 *   ATON_OSAL_LOG_LINES lines are kept; older ones are overwritten.
 *
 ****************************************************************************/

void aton_osal_nuttx_log_dump(void)
{
  unsigned int total = g_aton_log_total;
  unsigned int first;
  unsigned int i;

  if (total == 0)
    {
      _info("ATON log: (empty)\n");
      return;
    }

  _info("ATON log: %u line(s) recorded, showing the last %u\n",
        total, total < ATON_OSAL_LOG_LINES ? total : ATON_OSAL_LOG_LINES);

  first = (total > ATON_OSAL_LOG_LINES) ? (total - ATON_OSAL_LOG_LINES) : 0;

  for (i = first; i < total; i++)
    {
      _info("  [%u] %s", i, g_aton_log[i % ATON_OSAL_LOG_LINES]);
    }
}

/****************************************************************************
 * Name: aton_osal_nuttx_panic
 *
 * Description:
 *   Panic notifier: print the recorded NPU log before the board goes down.
 *   This runs with interrupts disabled in the middle of the panic, so it only
 *   walks the ring buffer and writes it out - no locks, no allocation.
 *
 ****************************************************************************/

static int aton_osal_nuttx_panic(FAR struct notifier_block *nb,
                                 unsigned long action, FAR void *data)
{
  UNUSED(nb);
  UNUSED(data);

  if (action == PANIC_TASK || action == PANIC_KERNEL)
    {
      aton_osal_nuttx_log_dump();
    }

  return OK;
}

/****************************************************************************
 * Name: aton_osal_nuttx_signal_event
 *
 * Description:
 *   Called from the NPU interrupt handler, therefore ISR-safe: only
 *   nxsem_post() is used, no logging.
 *
 ****************************************************************************/

void aton_osal_nuttx_signal_event(void)
{
  g_aton_event_count++;
  g_aton_stage = ATON_STAGE_NPU_ISR;
  (void)nxsem_post(&g_aton_wfe_sem);
}

/****************************************************************************
 * Name: aton_osal_nuttx_event_count
 *
 * Description:
 *   Return the number of epoch events signalled since boot.  Used by the
 *   driver to report how many NPU interrupts one inference needed (0 means
 *   the epoch loop never had to wait, i.e. the NPU did not use the
 *   interrupt path).
 *
 ****************************************************************************/

uint32_t aton_osal_nuttx_event_count(void)
{
  return g_aton_event_count;
}

uint32_t aton_osal_nuttx_irq_count(void)
{
  return g_aton_irq_count;
}

bool aton_osal_nuttx_storm(void)
{
  return g_aton_irq_storm;
}

int aton_osal_nuttx_stage(void)
{
  return g_aton_stage;
}

/****************************************************************************
 * Name: aton_osal_nuttx_report
 *
 * Description:
 *   One-line status of the runtime, printable from task context while an
 *   inference is in flight.  Tells "slow" from "stuck": if irqs/events/wfe
 *   keep moving the NPU is working, if they are frozen the stage name says
 *   which phase it died in.
 *
 ****************************************************************************/

void aton_osal_nuttx_report(FAR const char *tag)
{
  if (!g_aton_osal_verbose)
    {
      return;
    }

  _info("ATON %s: stage=%s irqs=%lu events=%lu wfe=%lu timeouts=%lu%s\n",
        (tag != NULL) ? tag : "status",
        aton_stage_name(g_aton_stage),
        (unsigned long)g_aton_irq_count,
        (unsigned long)g_aton_event_count,
        (unsigned long)g_aton_wfe_count,
        (unsigned long)g_aton_wfe_timeouts,
        g_aton_irq_storm ? " STORM-MASKED" : "");
}

/****************************************************************************
 * Name: aton_osal_nuttx_lock
 * Name: aton_osal_nuttx_unlock
 *
 * Description:
 *   Serialize ATON epochs.  The FreeRTOS port implements a hand-made
 *   "priority based" lock and documents the missing priority inheritance as
 *   its main downside; nxmutex provides inheritance natively, so the
 *   semantics are the same or better with far less state.
 *
 ****************************************************************************/

void aton_osal_nuttx_lock(void)
{
  g_aton_stage = ATON_STAGE_LOCK;
  nxmutex_lock(&g_aton_epoch_mutex);
}

void aton_osal_nuttx_unlock(void)
{
  g_aton_stage = ATON_STAGE_UNLOCK;
  nxmutex_unlock(&g_aton_epoch_mutex);
}

/****************************************************************************
 * Name: aton_osal_nuttx_cache_lock
 * Name: aton_osal_nuttx_cache_unlock
 *
 * Description:
 *   Plain mutual exclusion for cache maintenance (MCU D-cache and NPU
 *   cache), matching aton_osal_freertos_lock()/unlock().
 *
 ****************************************************************************/

void aton_osal_nuttx_cache_lock(void)
{
  nxmutex_lock(&g_aton_cache_mutex);
}

void aton_osal_nuttx_cache_unlock(void)
{
  nxmutex_unlock(&g_aton_cache_mutex);
}

/****************************************************************************
 * Name: aton_osal_nuttx_enter_cs
 * Name: aton_osal_nuttx_exit_cs
 *
 * Description:
 *   Mask the ATON interrupt line for the duration of a critical section
 *   (ll_aton_osal.h: "here we need to block also execution of IRQ
 *   handler").
 *
 ****************************************************************************/

void aton_osal_nuttx_enter_cs(void)
{
  g_aton_stage = ATON_STAGE_CS_ENTER;
  up_disable_irq(ATON_OSAL_NUTTX_IRQ(ATON_STD_IRQ_LINE));
}

void aton_osal_nuttx_exit_cs(void)
{
  g_aton_stage = ATON_STAGE_CS_EXIT;

  /* Do not undo a storm mask: the runtime is already off the rails and the
   * watchdog is reporting it.
   */

  if (!g_aton_irq_storm)
    {
      up_enable_irq(ATON_OSAL_NUTTX_IRQ(ATON_STD_IRQ_LINE));
    }
}

/****************************************************************************
 * Name: aton_osal_nuttx_dsb
 *
 * Description:
 *   Data synchronization barrier (the middleware's default implementation
 *   uses CMSIS __DSB()).
 *
 ****************************************************************************/

void aton_osal_nuttx_dsb(void)
{
  __asm__ __volatile__("dsb sy" : : : "memory");
}

/****************************************************************************
 * Name: aton_osal_nuttx_enable_irq
 ****************************************************************************/

void aton_osal_nuttx_enable_irq(int line, bool enable)
{
  if (line < 0 || line > 3)
    {
      _err("ERROR: invalid ATON IRQ line %d\n", line);
      return;
    }

  if (enable)
    {
      g_aton_stage = ATON_STAGE_IRQ_ENABLE;

      if (!g_aton_irq_storm)
        {
          up_enable_irq(ATON_OSAL_NUTTX_IRQ(line));
        }
    }
  else
    {
      g_aton_stage = ATON_STAGE_IRQ_DISABLE;
      up_disable_irq(ATON_OSAL_NUTTX_IRQ(line));
    }
}

/****************************************************************************
 * Name: aton_osal_nuttx_irq_adapter
 *
 * Description:
 *   NuttX ISR signature adapter.  The ATON runtime handler
 *   (ATON_STD_IRQHandler, i.e. NPU0_IRQHandler for line 0) takes no
 *   arguments, so it is passed as the ISR argument and invoked here.
 *
 *   The handler clears the ATON interrupt controller status and ends with
 *   LL_ATON_OSAL_SIGNAL_EVENT(), which posts the WFE semaphore.  It stays
 *   silent: the NSH console is interrupt driven on this board, so logging
 *   from this context freezes the shell.
 *
 ****************************************************************************/

static int aton_osal_nuttx_irq_adapter(int irq, FAR void *context,
                                       FAR void *arg)
{
  void (*handler)(void) = (void (*)(void))arg;
  uint32_t now;

  UNUSED(irq);
  UNUSED(context);

  g_aton_irq_count++;
  now = clock_systime_ticks();

  if ((uint32_t)(now - g_aton_irq_window) >= ATON_OSAL_IRQ_STORM_WINDOW)
    {
      g_aton_irq_window    = now;
      g_aton_irq_in_window = 1;
    }
  else if (++g_aton_irq_in_window > ATON_OSAL_IRQ_STORM_LIMIT)
    {
      /* Too many interrupts in the window: the line is stuck, not making
       * progress.  Masking it is the only way to get the CPU back to thread
       * mode so the runtime (and its watchdog) can run again.  The recorded
       * error details are dumped from the watchdog afterwards - printing here
       * is not allowed.
       */

      if (!g_aton_irq_storm)
        {
          g_aton_irq_storm = true;
          up_disable_irq(ATON_OSAL_NUTTX_IRQ(ATON_STD_IRQ_LINE));
        }

      return OK;
    }

  if (handler != NULL)
    {
      handler();
    }

  return OK;
}

/****************************************************************************
 * Name: aton_osal_nuttx_install_irq
 *
 * Description:
 *   Called by LL_ATON_RT_RuntimeInit() to install the runtime interrupt
 *   handler for one ATON line (line 0 = NPU0 = NVIC 53 by default).
 *
 ****************************************************************************/

void aton_osal_nuttx_install_irq(int line, void (*handler)(void))
{
  int ret;

  if (line < 0 || line > 3)
    {
      _err("ERROR: invalid ATON IRQ line %d\n", line);
      return;
    }

  g_aton_stage = ATON_STAGE_IRQ_INSTALL;

  ret = irq_attach(ATON_OSAL_NUTTX_IRQ(line), aton_osal_nuttx_irq_adapter,
                   (FAR void *)handler);
  if (ret < 0)
    {
      _err("ERROR: irq_attach(line %d) failed: %d\n", line, ret);
    }
}

/****************************************************************************
 * Name: aton_osal_nuttx_remove_irq
 ****************************************************************************/

void aton_osal_nuttx_remove_irq(int line)
{
  uint32_t irq;

  if (line < 0 || line > 3)
    {
      _err("ERROR: invalid ATON IRQ line %d\n", line);
      return;
    }

  irq = ATON_OSAL_NUTTX_IRQ(line);

  /* Do not detach: a detached IRQ falls through to the "unexpected
   * interrupt" handler, which logs to the console from interrupt context --
   * on this board that fights the interrupt-driven NSH console and freezes
   * the shell.  Leave a silent no-op handler installed instead (the adapter
   * ignores a NULL handler) and make sure the line is masked.
   */

  up_disable_irq(irq);
  (void)irq_attach(irq, aton_osal_nuttx_irq_adapter, NULL);
}

/****************************************************************************
 * Name: aton_osal_nuttx_set_priority
 *
 * Description:
 *   NVIC priority for an ATON interrupt line.  The NPU interrupt handler
 *   posts a semaphore (nxsem_post), which NuttX only permits from an
 *   interrupt of "normal" priority, so the runtime must not raise the NPU
 *   lines above the kernel's interrupt-priority ceiling.
 *
 ****************************************************************************/

void aton_osal_nuttx_set_priority(int line, int prio)
{
  if (line < 0 || line > 3)
    {
      _err("ERROR: invalid ATON IRQ line %d\n", line);
      return;
    }

  up_prioritize_irq(ATON_OSAL_NUTTX_IRQ(line), prio);
}

#endif /* CONFIG_LIB_AI_ATON */
