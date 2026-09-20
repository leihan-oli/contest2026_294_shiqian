/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_lcd.c
 *
 * Minimal LCD lower-half driver for the ATK-DNN647 7" RGB panel.
 *
 * The LTDC already scans the shared framebuffer g_ltdc_fb (exposed as
 * /dev/fb0).  This driver exposes the SAME buffer through the standard
 * NuttX LCD interface (/dev/lcd0) so that LVGL can use its LCD path
 * (intermediate draw buffer + PUTAREA), which dramatically reduces
 * tearing compared to the direct-framebuffer (DIRECT render) path.
 *
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/types.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <debug.h>
#include <syslog.h>

#include <nuttx/lcd/lcd.h>
#include <nuttx/board.h>
#include <nuttx/sched.h>
#include <nuttx/cache.h>

#include "arm_internal.h"
#include "hardware/stm32_ltdc.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_LCD_XRES    800
#define STM32N6_LCD_YRES    480
#define STM32N6_LCD_BPP     16
#define STM32N6_LCD_STRIDE  (STM32N6_LCD_XRES * (STM32N6_LCD_BPP >> 3))
#define STM32N6_LCD_FBSIZE  (STM32N6_LCD_STRIDE * STM32N6_LCD_YRES)

/* Scanline guard for tear-free partial updates.
 *
 * The LTDC scans the framebuffer continuously; a plain memcpy into a
 * region the scanline is currently crossing shows up as a horizontal
 * tear line (top of the region already shows the new pixels, the rest
 * still the old ones, or vice versa).  Before writing we therefore wait
 * until the scanline is clear of the target rows.
 *
 * STM32N6 LTDC exposes the current scan position in LTDC_CPSR.CYPOS
 * (unlike F4/H7 which cannot read it), so no interrupt is needed.
 * Framebuffer row 0 maps to CPSR Y = BPCR.AVBP + 1 (24 with the current
 * timing); the active area ends at Y = 24 + 480 = 504 of a 525-line
 * frame.  Guard lines give the memcpy head-room before the scanline can
 * reach the region again (24 lines ~= 760us at 60 Hz, well above the
 * ~160us worst-case full-width copy). */

#define STM32N6_LCD_SCAN_GUARD   24
#define STM32N6_LCD_CPSR_Y_MASK  0x0fff

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* LTDC framebuffer owned by the board (also /dev/fb0). */

extern uint16_t g_ltdc_fb[STM32N6_LCD_XRES * STM32N6_LCD_YRES];

/* PUTAREA diagnostics (report every PUTAREA_DIAG_INTERVAL calls):
 *   calls       - number of putarea invocations
 *   avg_wait    - average scanline busy-wait iterations (0 means the
 *                 scanline was already clear -> no wait at all)
 *   max_wait    - worst-case wait iterations (>= SCAN_WAIT_BUDGET means
 *                 the wait timed out, i.e. CPSR readback is unreliable)
 *   timeouts    - how many waits hit the budget
 * A large avg_wait with animation on (and near-zero after lv_anim_delete_all)
 * proves the "render can't keep up with the scanline" theory.
 */

#define PUTAREA_DIAG_INTERVAL  300
#define SCAN_WAIT_BUDGET       3000

static uint32_t g_lcd_putarea_calls;
static uint32_t g_lcd_wait_iters;
static uint32_t g_lcd_wait_iters_max;
static uint32_t g_lcd_wait_timeouts;
static uint32_t g_lcd_putarea_cycles;
static uint32_t g_lcd_putarea_cycles_max;

/* DWT cycle counter (Cortex-M), for high-resolution putarea timing.
 * DEMCR@0xE000EDFC TRCENA=bit24, DWT.CTRL@0xE0001000 CYCCNTENA=bit0,
 * DWT.CYCCNT@0xE0001004.  ~7.1s wrap at 600MHz. */

#define STM32N6_LCD_DEMCR     0xe000edfcu
#define STM32N6_LCD_DWT_CTRL  0xe0001000u
#define STM32N6_LCD_DWT_CYCCNT 0xe0001004u

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32n6_lcd_getvideoinfo(FAR struct lcd_dev_s *dev,
                                    FAR struct fb_videoinfo_s *vinfo);
static int stm32n6_lcd_getplaneinfo(FAR struct lcd_dev_s *dev,
                                    unsigned int planeno,
                                    FAR struct lcd_planeinfo_s *pinfo);
static int stm32n6_lcd_getarea(FAR struct lcd_dev_s *dev,
                               fb_coord_t row_start, fb_coord_t row_end,
                               fb_coord_t col_start, fb_coord_t col_end,
                               FAR uint8_t *buffer, fb_coord_t stride);
static int stm32n6_lcd_putarea(FAR struct lcd_dev_s *dev,
                               fb_coord_t row_start, fb_coord_t row_end,
                               fb_coord_t col_start, fb_coord_t col_end,
                               FAR const uint8_t *buffer, fb_coord_t stride);
static int stm32n6_lcd_getrun(FAR struct lcd_dev_s *dev, fb_coord_t row,
                              fb_coord_t col, FAR uint8_t *buffer,
                              size_t npixels);
static int stm32n6_lcd_putrun(FAR struct lcd_dev_s *dev, fb_coord_t row,
                              fb_coord_t col, FAR const uint8_t *buffer,
                              size_t npixels);
static int stm32n6_lcd_setpower(FAR struct lcd_dev_s *dev, int power);
static int stm32n6_lcd_getpower(FAR struct lcd_dev_s *dev);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int stm32n6_lcd_getvideoinfo(FAR struct lcd_dev_s *dev,
                                    FAR struct fb_videoinfo_s *vinfo)
{
  DEBUGASSERT(dev != NULL && vinfo != NULL);

  vinfo->fmt     = FB_FMT_RGB16_565;
  vinfo->xres    = STM32N6_LCD_XRES;
  vinfo->yres    = STM32N6_LCD_YRES;
  vinfo->nplanes = 1;
  return OK;
}

static int stm32n6_lcd_getplaneinfo(FAR struct lcd_dev_s *dev,
                                    unsigned int planeno,
                                    FAR struct lcd_planeinfo_s *pinfo)
{
  DEBUGASSERT(dev != NULL && pinfo != NULL);

  if (planeno == 0)
    {
      pinfo->putrun  = stm32n6_lcd_putrun;
      pinfo->getrun  = stm32n6_lcd_getrun;
      pinfo->putarea = stm32n6_lcd_putarea;
      pinfo->getarea = stm32n6_lcd_getarea;
      pinfo->buffer  = (FAR uint8_t *)g_ltdc_fb;
      pinfo->bpp     = STM32N6_LCD_BPP;
      pinfo->dev     = dev;
      return OK;
    }

  return -ENODEV;
}

static int stm32n6_lcd_getarea(FAR struct lcd_dev_s *dev,
                               fb_coord_t row_start, fb_coord_t row_end,
                               fb_coord_t col_start, fb_coord_t col_end,
                               FAR uint8_t *buffer, fb_coord_t stride)
{
  FAR const uint8_t *src;
  fb_coord_t cols;
  fb_coord_t rows;
  fb_coord_t row;
  size_t step;

  DEBUGASSERT(dev != NULL && buffer != NULL);

  cols = col_end - col_start + 1;
  rows = row_end - row_start + 1;
  step = (stride > 0) ? (size_t)stride : (size_t)(cols * 2);

  src = (FAR const uint8_t *)g_ltdc_fb +
        ((size_t)row_start * STM32N6_LCD_STRIDE) + (size_t)(col_start * 2);

  for (row = 0; row < rows; row++)
    {
      memcpy(buffer + row * step, src, (size_t)cols * 2);
      src += STM32N6_LCD_STRIDE;
    }

  return OK;
}

/****************************************************************************
 * Name: stm32n6_lcd_wait_scanline_safe
 *
 * Description:
 *   Busy-wait until the LTDC scanline is clear of the row range that is
 *   about to be written, so the memcpy lands in pixels the display has
 *   already read out and will only re-read on the next frame.  This is
 *   what eliminates mid-screen tearing for partial (PUTAREA) updates
 *   without needing a second framebuffer.
 *
 *   The scanline is "safe" when it is either below the region (CPSR Y >
 *   hi) or, after wrapping around the bottom of the frame, above it
 *   (CPSR Y < lo).  For regions near the bottom of the screen "below"
 *   never happens within a frame (hi clamps to TOTALH), so we wait for
 *   the wrap instead - the unified condition handles both cases.
 *
 *   Bounded: if the scan position cannot be read (LTDC disabled, or the
 *   CPSR readback is not reliable), we give up after a short budget and
 *   let the caller proceed with the (possibly tearing) write rather than
 *   hang the LVGL task.
 *
 ****************************************************************************/

static uint32_t stm32n6_lcd_wait_scanline_safe(fb_coord_t row_start,
                                               fb_coord_t row_end)
{
  uint32_t cypos;
  uint32_t total_h;
  uint32_t offset;
  int32_t lo;
  int32_t hi;
  int i;

  /* If the LTDC is not scanning there is nothing to synchronize with. */

  if ((getreg32(STM32_LTDC_GCR) & LTDC_GCR_LTDCEN) == 0)
    {
      return 0;
    }

  /* Frame geometry from the live registers (same values the LTDC driver
   * programmed): total height and the CPSR Y offset of framebuffer row 0
   * (= AVBP + 1). */

  total_h = (getreg32(STM32_LTDC_TWCR) & LTDC_TWCR_TOTALH_MASK) >>
            LTDC_TWCR_TOTALH_SHIFT;
  offset  = ((getreg32(STM32_LTDC_BPCR) & LTDC_BPCR_AVBP_MASK) >>
             LTDC_BPCR_AVBP_SHIFT) + 1;

  if (total_h == 0)
    {
      return 0;   /* No valid timing programmed; nothing to sync against. */
    }

  /* Unsafe scanline window around the target rows, clamped to the frame. */

  lo = (int32_t)(offset + row_start) - STM32N6_LCD_SCAN_GUARD;
  hi = (int32_t)(offset + row_end) + STM32N6_LCD_SCAN_GUARD;
  if (lo < 0)
    {
      lo = 0;
    }

  if (hi >= (int32_t)total_h)
    {
      hi = (int32_t)total_h;   /* Never "below"; wait for the wrap instead. */
    }

  for (i = 0; i < SCAN_WAIT_BUDGET; i++)
    {
      cypos = getreg32(STM32_LTDC_CPSR) & STM32N6_LCD_CPSR_Y_MASK;
      if ((int32_t)cypos < lo || (int32_t)cypos > hi)
        {
          break;
        }
    }

  return (uint32_t)i;
}

/****************************************************************************
 * Name: stm32n6_lcd_cache_diag
 *
 * Description:
 *   One-shot diagnostic (first putarea): determine why the framebuffer
 *   memcpy is so slow (~19ms for 160KB = ~285 cycles/word).  Compares
 *   framebuffer read/write speed against a plain static buffer and dumps
 *   SCTLR (D-Cache enable), MPU_CTRL and MAIR (cache attributes).  If
 *   fbW/fbR is ~100x scratchW/scratchR, the framebuffer region is
 *   non-cacheable (or write-through), so every CPU store goes to the bus.
 *
 ****************************************************************************/

#define LCD_DIAG_NWORDS  2048   /* 8KB each */

static uint32_t g_lcd_diag_scratch[LCD_DIAG_NWORDS] aligned_data(32);

static void stm32n6_lcd_cache_diag(void)
{
  uint32_t sctlr;
  uint32_t mpu_ctrl;
  uint32_t mair0;
  uint32_t mair1;
  uint32_t t0;
  uint32_t c_wfb;
  uint32_t c_rfb;
  uint32_t c_ws;
  uint32_t c_rs;
  uint32_t i;
  FAR uint32_t *fb = (FAR uint32_t *)g_ltdc_fb;
  volatile uint32_t sink = 0;

  sctlr    = getreg32(0xe000ed00);   /* SCTLR, bit2 = C (D-Cache) */
  mpu_ctrl = getreg32(0xe000ed94);   /* MPU_CTRL */
  mair0    = getreg32(0xe000edc0);   /* MAIR0 */
  mair1    = getreg32(0xe000edc4);   /* MAIR1 */

  t0 = getreg32(STM32N6_LCD_DWT_CYCCNT);
  for (i = 0; i < LCD_DIAG_NWORDS; i++)
    {
      fb[i] = 0x12345678u;
    }

  c_wfb = getreg32(STM32N6_LCD_DWT_CYCCNT) - t0;

  t0 = getreg32(STM32N6_LCD_DWT_CYCCNT);
  for (i = 0; i < LCD_DIAG_NWORDS; i++)
    {
      sink += fb[i];
    }

  c_rfb = getreg32(STM32N6_LCD_DWT_CYCCNT) - t0;

  t0 = getreg32(STM32N6_LCD_DWT_CYCCNT);
  for (i = 0; i < LCD_DIAG_NWORDS; i++)
    {
      g_lcd_diag_scratch[i] = 0x87654321u;
    }

  c_ws = getreg32(STM32N6_LCD_DWT_CYCCNT) - t0;

  t0 = getreg32(STM32N6_LCD_DWT_CYCCNT);
  for (i = 0; i < LCD_DIAG_NWORDS; i++)
    {
      sink += g_lcd_diag_scratch[i];
    }

  c_rs = getreg32(STM32N6_LCD_DWT_CYCCNT) - t0;

  syslog(LOG_INFO,
         "LCD cache diag: SCTLR=0x%08lx(C=%lu) MPU_CTRL=0x%08lx "
         "MAIR0=0x%08lx MAIR1=0x%08lx\n",
         (unsigned long)sctlr, (unsigned long)((sctlr >> 2) & 1),
         (unsigned long)mpu_ctrl, (unsigned long)mair0, (unsigned long)mair1);
  syslog(LOG_INFO,
         "LCD cache diag: fbW=%lu fbR=%lu | scratchW=%lu scratchR=%lu "
         "cycles (8KB each)\n",
         (unsigned long)c_wfb, (unsigned long)c_rfb,
         (unsigned long)c_ws, (unsigned long)c_rs);
}

static int stm32n6_lcd_putarea(FAR struct lcd_dev_s *dev,
                               fb_coord_t row_start, fb_coord_t row_end,
                               fb_coord_t col_start, fb_coord_t col_end,
                               FAR const uint8_t *buffer, fb_coord_t stride)
{
  FAR uint8_t *dst;
  FAR uint8_t *dst_start;
  fb_coord_t cols;
  fb_coord_t rows;
  fb_coord_t row;
  size_t step;
  uint32_t t0;

  DEBUGASSERT(dev != NULL && buffer != NULL);

  cols = col_end - col_start + 1;
  rows = row_end - row_start + 1;
  step = (stride > 0) ? (size_t)stride : (size_t)(cols * 2);

  dst = (FAR uint8_t *)g_ltdc_fb +
        ((size_t)row_start * STM32N6_LCD_STRIDE) + (size_t)(col_start * 2);
  dst_start = dst;

  /* Wait until the scanline has passed this region, then hold off task
   * preemption while copying so the copy completes within the scanline
   * guard margin. */

  {
    uint32_t waited = stm32n6_lcd_wait_scanline_safe(row_start, row_end);

    /* One-shot DWT enable on first call (idempotent, cheap). */

    if (g_lcd_putarea_calls == 0)
      {
        modifyreg32(STM32N6_LCD_DEMCR, 0, (1 << 24));       /* TRCENA */
        putreg32(0, STM32N6_LCD_DWT_CYCCNT);
        modifyreg32(STM32N6_LCD_DWT_CTRL, 0, 1);            /* CYCCNTENA */
        stm32n6_lcd_cache_diag();
      }

    t0 = getreg32(STM32N6_LCD_DWT_CYCCNT);

    g_lcd_putarea_calls++;
    g_lcd_wait_iters += waited;
    if (waited > g_lcd_wait_iters_max)
      {
        g_lcd_wait_iters_max = waited;
      }

    if (waited >= SCAN_WAIT_BUDGET)
      {
        g_lcd_wait_timeouts++;
      }

    if ((g_lcd_putarea_calls % PUTAREA_DIAG_INTERVAL) == 0)
      {
        syslog(LOG_INFO,
               "LCD putarea: calls=%lu avg_ms=%lu max_ms=%lu avg_wait=%lu "
               "timeouts=%lu\n",
               (unsigned long)g_lcd_putarea_calls,
               (unsigned long)(g_lcd_putarea_cycles /
                               g_lcd_putarea_calls / 600000),
               (unsigned long)(g_lcd_putarea_cycles_max / 600000),
               (unsigned long)(g_lcd_wait_iters / g_lcd_putarea_calls),
               (unsigned long)g_lcd_wait_timeouts);
      }
  }

  sched_lock();

  /* Fast path: a region spanning the full line width is contiguous in
   * both the source (step == cols*2) and the destination, so copy it
   * with a single memcpy instead of row-by-row.
   */

  if (step == (size_t)(cols * 2) && (size_t)(cols * 2) == STM32N6_LCD_STRIDE)
    {
      memcpy(dst, buffer, (size_t)cols * 2 * rows);
    }
  else
    {
      for (row = 0; row < rows; row++)
        {
          memcpy(dst, buffer + row * step, (size_t)cols * 2);
          dst += STM32N6_LCD_STRIDE;
        }
    }

  sched_unlock();

  /* Flush the written framebuffer region so the LTDC DMA reads the latest
   * pixels.  This is a no-op if the framebuffer is already non-cacheable,
   * but it is required when the region is cacheable -- otherwise the LTDC
   * (which does not go through the D-Cache) reads stale memory. */

  up_flush_dcache((uintptr_t)dst_start,
                  (uintptr_t)dst_start + (size_t)rows * STM32N6_LCD_STRIDE);

  /* Accumulate the total putarea wall time (scanline wait + memcpy). */

  {
    uint32_t cycles = getreg32(STM32N6_LCD_DWT_CYCCNT) - t0;

    g_lcd_putarea_cycles += cycles;
    if (cycles > g_lcd_putarea_cycles_max)
      {
        g_lcd_putarea_cycles_max = cycles;
      }
  }

  return OK;
}

static int stm32n6_lcd_getrun(FAR struct lcd_dev_s *dev, fb_coord_t row,
                              fb_coord_t col, FAR uint8_t *buffer,
                              size_t npixels)
{
  return stm32n6_lcd_getarea(dev, row, row, col, col + npixels - 1,
                             buffer, npixels * 2);
}

static int stm32n6_lcd_putrun(FAR struct lcd_dev_s *dev, fb_coord_t row,
                              fb_coord_t col, FAR const uint8_t *buffer,
                              size_t npixels)
{
  return stm32n6_lcd_putarea(dev, row, row, col, col + npixels - 1,
                             buffer, npixels * 2);
}

static int stm32n6_lcd_setpower(FAR struct lcd_dev_s *dev, int power)
{
  /* The LTDC keeps the panel powered; nothing to do here. */

  return OK;
}

static int stm32n6_lcd_getpower(FAR struct lcd_dev_s *dev)
{
  return CONFIG_LCD_MAXPOWER;
}

/****************************************************************************
 * Public Data
 ****************************************************************************/

struct lcd_dev_s g_stm32n6_lcd =
{
  /* LCD interface */

  .getvideoinfo = stm32n6_lcd_getvideoinfo,
  .getplaneinfo = stm32n6_lcd_getplaneinfo,
  .setpower     = stm32n6_lcd_setpower,
  .getpower     = stm32n6_lcd_getpower,
};
