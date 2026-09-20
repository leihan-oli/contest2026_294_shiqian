/****************************************************************************
 * apps/examples/eye_cam/eye_cam_main.c
 *
 * Live blink classification on the STM32N6 ATON NPU: the camera frame goes
 * through the eye ROI, the NPU and back onto the panel as a box plus a
 * class label.
 *
 * Why the chain below is written the way it is - every step has to reproduce
 * exactly what the model was trained on, and three of these are silent when
 * they are wrong (no error anywhere, the accuracy just drops):
 *
 *   capture   cam_save wrote the training set as 300x145 RGB BMPs, cut out of
 *             the 800x480 frame at x=235 y=155 (see
 *             主线流程_参数与采集协议.md: "最终 ROI").  The same rect is what
 *             eye_guide draws on screen, so the framing is already pinned.
 *
 *   colour    cam_save expands RGB565 with (v * 255) / 31 (and / 63 for
 *             green) - an exact linear map, not the usual bit replication
 *             ((v << 3) | (v >> 2)); the two differ by one for values such as
 *             r=17, so the same formula is used here.
 *
 *   resize    the training pipeline is PIL Image.BILINEAR, which for a
 *             downscale is a triangle filter whose support widens with the
 *             reduction factor.  eye_cam_build_taps() reproduces PIL's
 *             precompute_coeffs() tap for tap, and the result is rounded and
 *             clamped to uint8 - PIL returns 8 bit samples, and the /255
 *             happens after it.
 *
 *   layout    Input_0_out_0 declares shape {1,64,128,3} but mem_shape
 *             {1,3,64,128} with CHPos_First: the bytes in RAM are PLANAR
 *             (all R, then all G, then all B).  The training side transposes
 *             to NCHW for the same reason.  Writing interleaved RGB here -
 *             which is what palm_cam does for the 995 model - would feed the
 *             NPU three half-pictures and still look like a working run.
 *
 *   quant     scale / zero point taken from the model's own buffer table
 *             rather than measured - they change whenever the model is
 *             regenerated (the 5-class one has 0.007135717/-13 in and
 *             0.129406050/-38 out, the 2-class one had different values).
 *             The constants below are cross-checked against the product by
 *             eye_ai_train/check_quant_match.py.
 *
 * The input buffer address is not a free choice: the epoch program resolved
 * its input address once, when it was published, so the frame has to be
 * written to the address the driver reports (STM32N6_ATON_APP_INPUT_BASE in
 * arch/arm/src/stm32n6/stm32n6_aton_aie.c).  Filling anything else produces a
 * result that never changes.
 *
 * Usage:
 *   eye_cam [iterations] [hold_ms]
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <malloc.h>
#include <nuttx/leds/userled.h>
#include <nuttx/version.h>
#include <sys/stat.h>
#include <time.h>
#include <syslog.h>

#include "eye_cam_font.h"

#include <sys/videoio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <stdbool.h>

#include <nuttx/arch.h>
#include <nuttx/cache.h>
#include <nuttx/clock.h>
#include <nuttx/aie/ai_engine.h>
#include <nuttx/aie/stm32n6_aton_aie.h>
#include <nuttx/video/fb.h>


/* Board buzzer: PD3, an active buzzer on a plain pin, driven straight from
 * the application (see stm32n6_buzzer.c).  It gives the eye verdicts a
 * voice without going anywhere near the audio subsystem.
 */

extern void stm32n6_buzzer_initialize(void);
extern void stm32n6_buzzer_beep(uint32_t ms);
extern void stm32n6_buzzer_beeps(int count, uint32_t on_ms, uint32_t gap_ms);
extern void stm32n6_buzzer_tone(uint32_t hz, uint32_t ms);
extern void stm32n6_buzzer_tone_async(uint32_t hz);
extern void stm32n6_buzzer_tone_stop(void);
#if defined(CONFIG_GRAPHICS_LVGL)
#  include <lvgl/lvgl.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The address the epoch program was published with.  The driver reports it
 * through STM32N6_ATON_CMD_GET_STATUS and that is what main() uses; this is
 * only the fallback for a driver old enough not to have the field.
 */

#define EYE_CAM_INPUT_ADDR      0x2436c000  /* both slots share this one:
                                             * see the driver's
                                             * STM32N6_ATON_APP_INPUT_BASE */
#define EYE_CAM_OUTPUT_ADDR     0x24358000  /* fallback: model owned outputs
                                             * are read where the driver says */

/* Frame and ROI, in the panel's coordinate system.  Identical to eye_guide's
 * GUIDE_ROI_* and to the capture command in the protocol document - changing
 * one without the other invalidates the dataset.
 */

#define EYE_CAM_FRAME_W         800
#define EYE_CAM_FRAME_H         480
#define EYE_CAM_ROI_X           235
#define EYE_CAM_ROI_Y           155
#define EYE_CAM_ROI_W           300
#define EYE_CAM_ROI_H           145

/* Model geometry: "Input_0_out_0", int8, memory NCHW {1,3,64,128}. */

#define EYE_CAM_NN_H            64
#define EYE_CAM_NN_W            128
#define EYE_CAM_NN_PLANE        (EYE_CAM_NN_H * EYE_CAM_NN_W)       /* 8192 */
#define EYE_CAM_IN_LEN          (3 * EYE_CAM_NN_PLANE)              /* 24576 */

/* The model's own quantisation parameters, copied from
 * LL_ATON_Input_Buffers_Info_Default() / ...Output_Buffers_Info_Default().
 */

#define EYE_CAM_IN_SCALE        0.0070742024f   /* v3 重训 2026-09-19 */
#define EYE_CAM_IN_ZP           (-14)
#define EYE_CAM_OUT_SCALE       0.17015758f     /* v3 重训 2026-09-19 */
#define EYE_CAM_OUT_ZP          (-34)

/* The class order and count come from eye_common.TASK_KEYWORDS["blink"]: the
 * training run numbers the classes in that order, so these have to be kept in
 * step with eye_ai_train/eye_common.py.  A mismatch permutes the labels
 * instead of raising an error anywhere, which is why start-up compares the
 * driver's output length against EYE_CAM_NCLS and refuses to run otherwise.
 */

#define EYE_CAM_CLS_CLOSED      0
#define EYE_CAM_CLS_OPEN        1
#define EYE_CAM_CLS_LEFT        2
#define EYE_CAM_CLS_RIGHT       3
#define EYE_CAM_CLS_OTHER       4
#define EYE_CAM_NCLS            5
#define EYE_CAM_OUT_LEN         EYE_CAM_NCLS

/* v/255 -> (x - 0.5) / 0.5  =  v/127.5 - 1, then / scale + zero point.  Kept
 * folded into one affine map: q = round(v * GAIN + BIAS).
 */

#define EYE_CAM_Q_GAIN          (1.108696f)
#define EYE_CAM_Q_BIAS          (-155.358692f)

/* PIL's bilinear support is 1.0, so the tap count is bounded by
 * 2 * max(1, in/out) + 2: 6 for both axes of this ROI.  Two spare slots keep
 * the bound honest without a second bounds check in the inner loop.
 */

#define EYE_CAM_MAX_TAPS        8

#define EYE_CAM_REDRAW_MS       50
#define EYE_CAM_HOLD_MS         80

#define EYE_CAM_COL_CLOSED      0xf800      /* red    */
#define EYE_CAM_COL_OPEN        0x07e0      /* green  */
#define EYE_CAM_COL_LEFT        0x07ff      /* cyan   */
#define EYE_CAM_COL_RIGHT       0xffe0      /* yellow */
#define EYE_CAM_COL_OTHER       0x8410      /* grey   */
#define EYE_CAM_COL_SHADOW      0x0000      /* black  */

/* Verdict label geometry.  The cache band in main() is derived from these two
 * so the band and the text can never drift apart again - the first version
 * hardcoded "ROI_Y - 24" in one place and "ROI_Y - 30" in the other, which
 * left the top six rows of every glyph in dirty cache lines the LTDC never
 * saw (the panel showed clipped text).  palm_cam sidesteps this by cleaning
 * the whole framebuffer; this one keeps the narrow band because it runs every
 * frame, so the band simply has to start above the label.
 */

/* Face-driven crop geometry.
 *
 * The width matters more than it looks.  The classifier tells left from
 * right by the wedge of sclera the iris uncovers at the outer corner, so
 * a box that cuts close to the corners throws away part of the very
 * signal being classified.  At 1.8 the corner sat about 20 px from the
 * edge of a 160 px box, and left/right read poorly on the board.
 *
 * 2.0 is the value the training data itself implies: the fixed ROI the
 * model was trained on is 300 x 145, and the reference pupil distance is
 * about 144, so 2.0 * 144 = 288 reproduces the training scale - and the
 * 145/300 height ratio below reproduces its shape.  The PC side checked
 * both 1.8 and 2.0 end to end; 2.0 is the one that also matches training.
 */

#define EYE_CAM_CROP_K          2.0f
#define EYE_CAM_CROP_H_SCALE    0.75f   /* bottom edge up by a quarter */

/* The eye keeps the same margin above it as before.  That margin is
 * what absorbs head movement, and an occlusion test on five captures
 * found the model barely reads it.  Raising the bottom edge while
 * holding the top one means the eye's fractional position grows by
 * the inverse of the height scale - hence 0.31 / 0.75.  The strip
 * that goes is the one the same test put at a fifth of the eyes'
 * contribution, most of that being an artefact of the mask fill.
 */

#define EYE_CAM_CROP_EYE_REL    0.413f
#define EYE_CAM_COLOR_CROP      0x07ff      /* cyan, no class uses it */

/* The verdict frame and its label follow the crop rectangle now that the
 * crop follows the face.  The label sits this far above the frame when
 * there is room, and the same distance below it when there is not.
 */

#define EYE_CAM_LABEL_GAP       30
#define EYE_CAM_VERDICT_PAD     3     /* frame drawn outside the crop */
#define EYE_CAM_DRAW_MARGIN     80    /* cache push covers this much */

#define EYE_CAM_LABEL_SCALE     4
#define EYE_CAM_LABEL_H         (7 * EYE_CAM_LABEL_SCALE + 1)   /* + shadow */
#define EYE_CAM_LABEL_Y         (EYE_CAM_ROI_Y - 30)
#define EYE_CAM_BAND_PAD        8           /* slack above the label */

#ifndef CONFIG_EXAMPLES_EYE_CAM_MOUNTPOINT
#  define CONFIG_EXAMPLES_EYE_CAM_MOUNTPOINT "/mnt/sdcard"
#endif

/* Dataset capture.  The crop the classifier is fed is written out at
 * full size, one BMP per frame, so the training images come off the
 * same rectangle, the same camera and the same expansion tables as the
 * inference does.  Rescaling to the model's tensor is left to the PC:
 * doing it here would mean a second resampler whose behaviour has to
 * be kept in step with PIL's.
 */

#define EYE_CAM_CAP_DIR   CONFIG_EXAMPLES_EYE_CAM_MOUNTPOINT "/eye_ds"
#define EYE_CAM_CAP_META  EYE_CAM_CAP_DIR "/meta.txt"
#define EYE_CAM_CAP_DEF_N 220

/* Fixed-frame captures use the size a face crop comes out at around a
 * typical working distance (dx 95 -> 190x70), so every class lands on
 * the same scale once resized to the tensor.
 */

#define EYE_CAM_CAP_FIXED_W 190
#define EYE_CAM_CAP_FIXED_H 70
/* Where the photo page drops full frames.  Kept away from eye_ds so a
 * training set can be wiped without taking the shots with it.
 */

#define EYE_UI_PHOTO_DIR  CONFIG_EXAMPLES_EYE_CAM_MOUNTPOINT "/photo"

#define EYE_CAM_CAP_MAX_W 512     /* sanity bounds on a crop */
#define EYE_CAM_CAP_MAX_H 512

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct eye_cam_tap_s
{
  int16_t idx;
  float   w;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static FAR uint16_t *g_fb;
static uint32_t      g_stride_px;

/* Face detector: its activations live in the pool the driver registered,
 * which is where they will keep landing, so the address and the span are
 * remembered from the preflight that ran the model once.
 */

static FAR int8_t   *g_face_out;
static uint32_t      g_face_out_len;
static int           g_face_ok;
static int           g_aiefd = -1;

/* Channel expansion tables: exactly cam_save's (v * 255) / 31 and / 63, so a
 * framebuffer pixel turns into the same 8 bit triple the BMP holds.
 */

static uint8_t g_exp5[32];
static uint8_t g_exp6[64];

/* Resampling taps, built once.  ~13 KB of .bss, which is cheaper than
 * recomputing 192 filters per frame.
 */

static struct eye_cam_tap_s g_htap[EYE_CAM_NN_W][EYE_CAM_MAX_TAPS];
static struct eye_cam_tap_s g_vtap[EYE_CAM_NN_H][EYE_CAM_MAX_TAPS];
static int                  g_hn[EYE_CAM_NN_W];
static int                  g_vn[EYE_CAM_NN_H];

/* The rectangle the classifier is fed.  It follows the detector when a
 * face is found and otherwise keeps the last one: a face does not move
 * far between two frames, and falling back to the fixed rectangle would
 * throw away a good localisation over one missed frame.
 */

static int  g_crop_x = EYE_CAM_ROI_X;
static int  g_crop_y = EYE_CAM_ROI_Y;
static int  g_crop_w = EYE_CAM_ROI_W;
static int  g_crop_h = EYE_CAM_ROI_H;
static int  g_crop_ok;              /* true once a face placed it */

/* What the resampler's taps were built for.  They depend only on the
 * crop size, and that creeps rather than jumps, so they are rebuilt on
 * change and reused in between.
 */

static int  g_tap_w = -1;
static int  g_tap_h = -1;

/* Capture state.  Zero frames means the app runs as it always did, so
 * nothing on the inference path changes when the mode is off.
 */

static FAR const char *g_cap_label;   /* class name in the file name */
static int             g_cap_count;   /* frames asked for            */
static int             g_cap_saved;   /* frames written              */
static int             g_cap_seq = 1; /* first free file number      */
static FAR FILE       *g_cap_meta;    /* meta.txt, append            */
static int             g_cap_fixed;   /* pin the crop, skip the face */
static int             g_cap_fixed_w = EYE_CAM_CAP_FIXED_W;
static int             g_cap_fixed_h = EYE_CAM_CAP_FIXED_H;

/* Label and colour per class, indexed by the model's output index.  Both the
 * box and the text take the colour, so the verdict reads from across the room
 * without having to spell it out.
 */

static FAR const char *g_eye_cam_name[EYE_CAM_NCLS] =
{
  "CLOSED", "OPEN", "LEFT", "RIGHT", "OTHER"
};

static const uint16_t g_eye_cam_color[EYE_CAM_NCLS] =
{
  EYE_CAM_COL_CLOSED, EYE_CAM_COL_OPEN, EYE_CAM_COL_LEFT,
  EYE_CAM_COL_RIGHT, EYE_CAM_COL_OTHER
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Diagnostics go through the driver's polled string channel: with the console
 * driver in the path a stalled TX semaphore would freeze the app, and this
 * one runs while the display is live.
 */

static void eye_cam_out(FAR const char *fmt, ...)
{
  char buf[176];
  va_list ap;
  int n;

  va_start(ap, fmt);
  n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (n <= 0)
    {
      return;
    }

  if (g_aiefd >= 0)
    {
      (void)ioctl(g_aiefd, STM32N6_ATON_CMD_PUTS, (unsigned long)buf);
    }
  else
    {
      fputs(buf, stdout);
      fflush(stdout);
    }
}

static void eye_cam_build_taps(int out_n, int in_n,
                               FAR struct eye_cam_tap_s *tap, FAR int *ntap)
{
  double ratio = (double)in_n / (double)out_n;
  double filterscale = ratio < 1.0 ? 1.0 : ratio;
  double support = filterscale;             /* bilinear support is 1.0 */
  double ss = 1.0 / filterscale;
  int    i;

  for (i = 0; i < out_n; i++)
    {
      FAR struct eye_cam_tap_s *row = tap + (size_t)i * EYE_CAM_MAX_TAPS;
      double center = ((double)i + 0.5) * ratio;
      int    xmin = (int)(center - support + 0.5);
      int    xmax = (int)(center + support + 0.5);
      double sum = 0.0;
      int    n = 0;
      int    x;

      if (xmin < 0)
        {
          xmin = 0;
        }

      if (xmax > in_n)
        {
          xmax = in_n;
        }

      for (x = xmin; x < xmax && n < EYE_CAM_MAX_TAPS; x++)
        {
          double t = fabs(((double)x - center) * ss);
          double w = t < 1.0 ? 1.0 - t : 0.0;

          row[n].idx = (int16_t)x;
          row[n].w   = (float)w;
          sum += w;
          n++;
        }

      if (sum != 0.0)
        {
          for (x = 0; x < n; x++)
            {
              row[x].w = (float)(row[x].w / sum);
            }
        }

      ntap[i] = n;
    }
}

/****************************************************************************
 * Name: eye_cam_taps_for
 *
 * Description:
 *   Make sure the taps describe a crop of this size.  The sizes come
 *   from the detector and only creep, so rebuilding on every change
 *   costs little.
 *
 ***************************************************************************/

static void eye_cam_taps_for(int cw, int ch)
{
  if (cw == g_tap_w && ch == g_tap_h)
    {
      return;
    }

  eye_cam_build_taps(EYE_CAM_NN_W, cw, &g_htap[0][0], g_hn);
  eye_cam_build_taps(EYE_CAM_NN_H, ch, &g_vtap[0][0], g_vn);

  g_tap_w = cw;
  g_tap_h = ch;
}

/* RGB565 800x480 (the buffer the DCMIPP fills and the LTDC scans out) ->
 * int8 3x64x128 planar model input.
 */

static void eye_cam_roi_to_input(FAR const uint16_t *fb, uint32_t stride,
                                 FAR int8_t *dst, int cx, int cy)
{
  int oy;

  memset(dst, 0, EYE_CAM_IN_LEN);

  for (oy = 0; oy < EYE_CAM_NN_H; oy++)
    {
      int ky;
      int ox;

      for (ox = 0; ox < EYE_CAM_NN_W; ox++)
        {
          float r = 0.0f;
          float g = 0.0f;
          float b = 0.0f;
          int   q;
          int   kx;

          for (ky = 0; ky < g_vn[oy]; ky++)
            {
              int   srow = cy + g_vtap[oy][ky].idx;
              FAR const uint16_t *row;
              float wy = g_vtap[oy][ky].w;

              /* Whatever falls outside the frame is black, which is what
               * PIL's crop gives for a box that hangs over an edge.  A
               * black sample is index 0 in every channel of the 565
               * expansion, so it can just be folded in at full weight -
               * the same as building the crop and resampling it.
               */

              if (srow < 0 || srow >= EYE_CAM_FRAME_H)
                {
                  continue;
                }

              row = fb + (size_t)srow * (stride / 2);

              for (kx = 0; kx < g_hn[ox]; kx++)
                {
                  int scol = cx + g_htap[ox][kx].idx;
                  uint16_t p = (scol < 0 || scol >= EYE_CAM_FRAME_W)
                               ? 0 : row[scol];
                  float w = wy * g_htap[ox][kx].w;

                  r += w * (float)g_exp5[(p >> 11) & 0x1f];
                  g += w * (float)g_exp6[(p >> 5) & 0x3f];
                  b += w * (float)g_exp5[p & 0x1f];
                }
            }

          /* PIL hands back uint8 samples (clip + round to nearest), and the
           * /255 and the model's quantiser see those.  Rounding here rather
           * than at the very end is what keeps the two chains identical.
           */

          r = (float)(r <= 0.0f ? 0 : (r >= 255.0f ? 255 : (int)(r + 0.5f)));
          g = (float)(g <= 0.0f ? 0 : (g >= 255.0f ? 255 : (int)(g + 0.5f)));
          b = (float)(b <= 0.0f ? 0 : (b >= 255.0f ? 255 : (int)(b + 0.5f)));

          /* Planar, in the order the mem_shape declares. */

          q = (int)lroundf(r * EYE_CAM_Q_GAIN + EYE_CAM_Q_BIAS);
          dst[ox + oy * EYE_CAM_NN_W] =
            (int8_t)(q < -128 ? -128 : (q > 127 ? 127 : q));

          q = (int)lroundf(g * EYE_CAM_Q_GAIN + EYE_CAM_Q_BIAS);
          dst[EYE_CAM_NN_PLANE + ox + oy * EYE_CAM_NN_W] =
            (int8_t)(q < -128 ? -128 : (q > 127 ? 127 : q));

          q = (int)lroundf(b * EYE_CAM_Q_GAIN + EYE_CAM_Q_BIAS);
          dst[2 * EYE_CAM_NN_PLANE + ox + oy * EYE_CAM_NN_W] =
            (int8_t)(q < -128 ? -128 : (q > 127 ? 127 : q));
        }
    }
}

/****************************************************************************
 * Drawing.  Same shape as eye_guide's and palm_cam's helpers: they write into
 * the buffer the LTDC scans out, because this app already holds the pointer
 * it reads the frame from.
 ****************************************************************************/

static void eye_cam_hline(int x, int y, int len, uint16_t color)
{
  FAR uint16_t *row;

  if (g_fb == NULL || len <= 0 || y < 0 || y >= EYE_CAM_FRAME_H)
    {
      return;
    }

  if (x < 0)
    {
      len += x;
      x = 0;
    }

  if (x + len > EYE_CAM_FRAME_W)
    {
      len = EYE_CAM_FRAME_W - x;
    }

  if (len <= 0)
    {
      return;
    }

  row = g_fb + (size_t)y * g_stride_px + (size_t)x;

  while (len-- > 0)
    {
      *row++ = color;
    }
}

static void eye_cam_vline(int x, int y, int len, uint16_t color)
{
  int i;

  for (i = 0; i < len; i++)
    {
      eye_cam_hline(x, y + i, 1, color);
    }
}

static void eye_cam_rect(int x, int y, int w, int h, int t, uint16_t color)
{
  int i;

  if (w <= 0 || h <= 0)
    {
      return;
    }

  for (i = 0; i < t; i++)
    {
      eye_cam_hline(x, y + i, w, color);
      eye_cam_hline(x, y + h - 1 - i, w, color);
      eye_cam_vline(x + i, y, h, color);
      eye_cam_vline(x + w - 1 - i, y, h, color);
    }
}

static void eye_cam_text(int x, int y, FAR const char *s, int scale,
                         uint16_t fg, uint16_t bg)
{
  for (; *s != '\0'; s++)
    {
      FAR const uint8_t *g = eye_cam_font_glyph(*s);
      int col;
      int row;

      for (col = 0; col < 5; col++)
        {
          for (row = 0; row < 7; row++)
            {
              uint16_t c = (g[col] & (1u << row)) ? fg : bg;
              int i;

              for (i = 0; i < scale; i++)
                {
                  eye_cam_hline(x + col * scale, y + row * scale + i,
                                scale, c);
                }
            }
        }

      x += 6 * scale;
    }
}

static void eye_cam_label(int x, int y, FAR const char *s, int scale,
                          uint16_t fg)
{
  eye_cam_text(x + 1, y + 1, s, scale, EYE_CAM_COL_SHADOW,
               EYE_CAM_COL_SHADOW);
  eye_cam_text(x, y, s, scale, fg, EYE_CAM_COL_SHADOW);
}

/****************************************************************************
 * Name: eye_cam_face_preflight
 *
 * Description:
 *   Query the second ATON slot (the YuNet face detector) and run its epoch
 *   program once.
 *
 *   The result of that one inference is meaningless - the input buffer holds
 *   whatever was there - but the ability to produce it is not: both models
 *   share the epoch controller and the input buffer address, and the two
 *   stage pipeline depends on them being runnable one after the other.  Say
 *   so at startup, not on the first frame of a collection run.
 *
 *   The buffer is left as the NPU saw it; the classifier overwrites the part
 *   it uses before its first inference.
 *
 * Returned Value:
 *   OK when the slot is present and its program ran; a negated errno value
 *   otherwise (the caller then carries on with the classifier alone).
 *
 ****************************************************************************/

static int eye_cam_face_preflight(int aiefd, FAR int8_t *input)
{
  struct stm32n6_aton_invoke_params params;
  struct stm32n6_aton_status_s status;
  int ret;

  memset(&status, 0, sizeof(status));
  status.model_id = STM32N6_ATON_MODEL_FACE;

  ret = ioctl(aiefd, STM32N6_ATON_CMD_GET_STATUS, (unsigned long)&status);
  if (ret < 0)
    {
      return -errno;
    }

  /* Two lines rather than one: the model name plus the tensor numbers do not
   * fit the polled writer's buffer without being cut short.
   */

  eye_cam_out("eye_cam: face slot \"%s\"\n", status.name);
  eye_cam_out("eye_cam: face slot: input %lu B, output %lu B at 0x%08lx\n",
              (unsigned long)status.input_size,
              (unsigned long)status.output_size,
              (unsigned long)status.output_addr);

  memset(&params, 0, sizeof(params));
  params.model_id    = STM32N6_ATON_MODEL_FACE;
  params.input       = input;
  params.output      = (FAR void *)(uintptr_t)
                       (status.output_addr != 0 ? status.output_addr
                                                : (uint32_t)(uintptr_t)input);
  params.input_size  = status.input_size;
  params.output_size = status.output_size;

  ret = ioctl(aiefd, AIE_CMD_FEED_INPUT, (unsigned long)&params);
  if (ret < 0)
    {
      return -errno;
    }

  eye_cam_out("eye_cam: face epoch program ran: %lu us, runs=%lu\n",
              (unsigned long)params.usec, (unsigned long)params.runs);

  /* Keep the output address the epoch program was published with: every
   * later frame's activations are written there too.
   */

  g_face_out     = (FAR int8_t *)params.output;
  g_face_out_len = status.output_size;
  return OK;
}

/****************************************************************************
 * Name: eye_cam_stage_add / eye_cam_stage_report
 *
 * Description:
 *   Per-stage frame timers.  One millisecond of resolution is deliberate: the
 *   stages worth attacking here are tens of milliseconds, and a stage that
 *   reports zero is not one of them.
 *
 *   The report is cumulative, so the numbers settle after a few frames and do
 *   not depend on which frame the log happens to catch.
 *
 ****************************************************************************/

struct eye_cam_stage_s
{
  FAR const char *name;
  uint32_t        sum;     /* Total ticks over all frames      */
  uint32_t        worst;   /* Longest single occurrence        */
  int             n;
};

enum eye_cam_stage_e
{
  EYE_CAM_ST_READ = 0,
  EYE_CAM_ST_STREAMOFF,
  EYE_CAM_ST_FACE,
  EYE_CAM_ST_CROP,
  EYE_CAM_ST_INFER,
  EYE_CAM_ST_OVERLAY,
  EYE_CAM_ST_HOLD,
  EYE_CAM_ST_STREAMON,
  EYE_CAM_ST_TOTAL,
  EYE_CAM_ST_NSTAGE
};

static struct eye_cam_stage_s g_stage[EYE_CAM_ST_NSTAGE] =
{
  { "read",      0, 0, 0 },
  { "streamoff", 0, 0, 0 },
  { "face",      0, 0, 0 },
  { "crop",      0, 0, 0 },
  { "infer",     0, 0, 0 },
  { "overlay",   0, 0, 0 },
  { "hold",      0, 0, 0 },
  { "streamon",  0, 0, 0 },
  { "total",     0, 0, 0 },
};

static void eye_cam_stage_add(int st, clock_t dt)
{
  uint32_t t = (uint32_t)dt;

  g_stage[st].sum += t;
  g_stage[st].n   += 1;

  if (t > g_stage[st].worst)
    {
      g_stage[st].worst = t;
    }
}

static void eye_cam_stage_report(int frames)
{
  int k;

  if (frames <= 0)
    {
      return;
    }

  eye_cam_out("eye_cam: --- per-stage ms over %d frame(s) ---\n", frames);

  for (k = 0; k < EYE_CAM_ST_NSTAGE; k++)
    {
      if (g_stage[k].n == 0)
        {
          continue;
        }

      eye_cam_out("eye_cam: %-9s avg %lu  worst %lu  n=%d\n",
                  g_stage[k].name,
                  (unsigned long)TICK2MSEC(g_stage[k].sum / g_stage[k].n),
                  (unsigned long)TICK2MSEC(g_stage[k].worst),
                  g_stage[k].n);
    }
}

/****************************************************************************
 * Face detection - the first half of the two stage pipeline
 *
 * The reference the board has to reproduce is
 *
 *   frame -> YuNet 256x416 int8 -> best anchor -> eye keypoints
 *         -> crop 1.8*dx wide with the eye at 0.31 of its height
 *         -> 128x64 int8 classifier
 *
 * This block is the first two steps.  The board runs the same network at the
 * same resolution, and the decoded quantity is the same one: the score of
 * the best anchor over all three strides, whose keypoint head then gives the
 * eye centre and the inter-pupillary distance the crop is sized from.
 *
 *   score = sqrt(clip(cls) * clip(obj))    (the product commutes, so either
 *                                           single value head can take
 *                                           either role)
 *   i     = argmax(score) over that stride's 1664 / 416 / 104 anchors
 *   no face this frame when the best score is below 0.6
 *
 * The reference never decodes a bounding box - it reads the keypoints of the
 * winning anchor directly - so the three box tensors are left alone and
 * there is no NMS here either.
 *
 * Keypoints are quantised as offsets from their anchor:
 *
 *   px = (anchor_col + kx) * stride * (800 / 416)
 *   py = (anchor_row + ky) * stride * (480 / 256)
 *
 * with kp[0] the right eye and kp[1] the left one, so kp[2..3] continues the
 * flat array with the left eye's x and y.
 ****************************************************************************/

#define EYE_CAM_FACE_H          256
#define EYE_CAM_FACE_W          416
#define EYE_CAM_FACE_PLANE      (EYE_CAM_FACE_H * EYE_CAM_FACE_W)
#define EYE_CAM_FACE_IN_LEN     (3 * EYE_CAM_FACE_PLANE)          /* 319488 */
#define EYE_CAM_FACE_THR        0.6f

/* Twelve tensors come out of the model.  Nine of them are used, three per
 * stride: the two single channel heads whose product is the score, and the
 * ten channel keypoint head.  The offsets are the ones ST's generator
 * assigned inside the pool, and the scales and zero points are the
 * per-tensor quantisation parameters from the same generator.
 */

struct eye_cam_face_scale_s
{
  uint32_t nanchor;
  uint32_t stride;
  uint32_t off_a;
  uint32_t off_b;
  uint32_t off_kps;
  float    sc_a;
  float    sc_b;
  float    sc_kps;
  int32_t  zp_a;
  int32_t  zp_b;
  int32_t  zp_kps;
};

static const struct eye_cam_face_scale_s g_face_scale[3] =
{
  { 1664,  8, 449280,  16640,      0,
    0.0036890279f, 0.0039136480f, 0.0380033851f, -128, -128,  -40 },
  {  416, 16, 575952, 576368, 565760,
    0.0023810691f, 0.0000037500f, 0.0050239060f, -128, -128, -112 },
  {  104, 32, 577200, 577312, 574912,
    0.0034092192f, 0.0039212201f, 0.0261975788f, -128, -128,  -35 },
};

struct eye_cam_face_s
{
  float    score;
  uint32_t stride;
  uint32_t anchor;
  float    ex;            /* Eye centre, frame pixels     */
  float    ey;
  float    dx;            /* Inter-pupillary distance     */
  float    kp[10];        /* Five keypoints, frame pixels */
};

/* The resampler taps only depend on the pixel index, so they are worked out
 * once instead of 106496 times per frame.
 */

static int   g_face_x0[EYE_CAM_FACE_W];
static float g_face_xw[EYE_CAM_FACE_W];
static int   g_face_y0[EYE_CAM_FACE_H];
static float g_face_yw[EYE_CAM_FACE_H];
static int   g_face_taps_ready;

/****************************************************************************
 * Name: eye_cam_face_taps
 *
 * Description:
 *   Work out where the frame -> 256x416 resampler reads from.
 *
 *   cv2 puts an output sample at
 *
 *     s = (d + 0.5) * scale - 0.5
 *
 *   and interpolates between the two source samples straddling it.  Doing
 *   the same keeps the board's input as close to the reference's as the
 *   sensor's 5-6-5 bit depth allows.
 *
 ***************************************************************************/

static void eye_cam_face_taps(void)
{
  float sx = (float)EYE_CAM_FRAME_W / (float)EYE_CAM_FACE_W;
  float sy = (float)EYE_CAM_FRAME_H / (float)EYE_CAM_FACE_H;
  int   d;

  for (d = 0; d < EYE_CAM_FACE_W; d++)
    {
      float s = ((float)d + 0.5f) * sx - 0.5f;
      int   i = (int)floorf(s);

      if (i < 0)
        {
          i = 0;
        }
      else if (i > EYE_CAM_FRAME_W - 2)
        {
          i = EYE_CAM_FRAME_W - 2;
        }

      g_face_x0[d] = i;
      g_face_xw[d] = s - (float)i;
    }

  for (d = 0; d < EYE_CAM_FACE_H; d++)
    {
      float s = ((float)d + 0.5f) * sy - 0.5f;
      int   i = (int)floorf(s);

      if (i < 0)
        {
          i = 0;
        }
      else if (i > EYE_CAM_FRAME_H - 2)
        {
          i = EYE_CAM_FRAME_H - 2;
        }

      g_face_y0[d] = i;
      g_face_yw[d] = s - (float)i;
    }

  g_face_taps_ready = 1;
}

/****************************************************************************
 * Name: eye_cam_face_prep
 *
 * Description:
 *   Turn the whole frame into the detector's input tensor: three int8
 *   planes of 256x416 in RGB order, quantised as (value - 128).  The model
 *   was calibrated with scale 1 and zero point -128 over the raw 0..255
 *   range, and the reference does not divide by 255 either.
 *
 *   The resampling is a plain 2x2 bilinear matching cv2.INTER_LINEAR.  It
 *   is deliberately not the separable multi-tap filter the eye crop uses:
 *   that one reproduces PIL's anti-aliased resize, which is a different
 *   algorithm, on a different kind of input, for a different model.
 *
 ***************************************************************************/

static void eye_cam_face_prep(FAR const uint16_t *fb, uint32_t stride,
                              FAR int8_t *dst)
{
  uint32_t rowoff = stride / 2;
  int      oy;

  if (!g_face_taps_ready)
    {
      eye_cam_face_taps();
    }

  for (oy = 0; oy < EYE_CAM_FACE_H; oy++)
    {
      FAR const uint16_t *row0 = fb + (size_t)g_face_y0[oy] * rowoff;
      FAR const uint16_t *row1 = row0 + rowoff;
      float  wy  = g_face_yw[oy];
      float  wy1 = 1.0f - wy;
      FAR int8_t *p0 = dst + (size_t)oy * EYE_CAM_FACE_W;
      FAR int8_t *p1 = p0 + EYE_CAM_FACE_PLANE;
      FAR int8_t *p2 = p1 + EYE_CAM_FACE_PLANE;
      int    ox;

      for (ox = 0; ox < EYE_CAM_FACE_W; ox++)
        {
          int   x0  = g_face_x0[ox];
          float wx  = g_face_xw[ox];
          float wx1 = 1.0f - wx;
          uint16_t a = row0[x0];
          uint16_t b = row0[x0 + 1];
          uint16_t c = row1[x0];
          uint16_t d = row1[x0 + 1];
          float    t;
          float    u;

          /* Red: bits 15..11.  The 565 channels are expanded with the same
           * tables the eye crop uses, so both stages agree on what a 565
           * pixel means.
           */

          t = (float)g_exp5[(a >> 11) & 0x1f] * wx1 +
              (float)g_exp5[(b >> 11) & 0x1f] * wx;
          u = (float)g_exp5[(c >> 11) & 0x1f] * wx1 +
              (float)g_exp5[(d >> 11) & 0x1f] * wx;
          p0[ox] = (int8_t)(lroundf(t * wy1 + u * wy) - 128);

          /* Green: bits 10..5 */

          t = (float)g_exp6[(a >> 5) & 0x3f] * wx1 +
              (float)g_exp6[(b >> 5) & 0x3f] * wx;
          u = (float)g_exp6[(c >> 5) & 0x3f] * wx1 +
              (float)g_exp6[(d >> 5) & 0x3f] * wx;
          p1[ox] = (int8_t)(lroundf(t * wy1 + u * wy) - 128);

          /* Blue: bits 4..0 */

          t = (float)g_exp5[a & 0x1f] * wx1 + (float)g_exp5[b & 0x1f] * wx;
          u = (float)g_exp5[c & 0x1f] * wx1 + (float)g_exp5[d & 0x1f] * wx;
          p2[ox] = (int8_t)(lroundf(t * wy1 + u * wy) - 128);
        }
    }
}

/****************************************************************************
 * Name: eye_cam_input_probe
 *
 * Description:
 *   Print the corners of the tensor the classifier is about to read.  The
 *   crop rectangle and the eye markers are drawn into the same buffer the
 *   frame lives in, and the frame is what this tensor is cut from, so the
 *   question of whether any of that leaked in is worth answering with a
 *   measurement rather than an argument about ordering.
 *
 *   The corners are the corners of the crop, so a drawn rectangle would
 *   show up in them; the middle is where the eye markers would be.  Skin
 *   is red-dominant, the markers are cyan (blue-green), and anything off
 *   the frame is black at -128.
 *
 ***************************************************************************/

static void eye_cam_input_probe(FAR const int8_t *in)
{
  int n = EYE_CAM_NN_PLANE;
  int w = EYE_CAM_NN_W;
  int h = EYE_CAM_NN_H;

  eye_cam_out("eye_cam: in TL %d %d %d   TR %d %d %d\n",
              in[0], in[n], in[2 * n],
              in[w - 1], in[n + w - 1], in[2 * n + w - 1]);
  eye_cam_out("eye_cam: in BL %d %d %d   BR %d %d %d\n",
              in[(h - 1) * w], in[n + (h - 1) * w],
              in[2 * n + (h - 1) * w],
              in[h * w - 1], in[n + h * w - 1], in[2 * n + h * w - 1]);
}

/****************************************************************************
 * Name: eye_cam_face_decode
 *
 * Description:
 *   Score every anchor of every stride, keep the best, and read its
 *   keypoints.  Returns true when a face was found, in which case *out is
 *   filled in even for a rejection so the caller can report the score.
 *
 ***************************************************************************/

static int eye_cam_face_decode(FAR const int8_t *pool,
                               FAR struct eye_cam_face_s *out)
{
  float    best_sc = -1.0f;
  uint32_t best_s  = 0;
  uint32_t best_i  = 0;
  int      s;

  memset(out, 0, sizeof(*out));

  for (s = 0; s < 3; s++)
    {
      FAR const struct eye_cam_face_scale_s *sc = &g_face_scale[s];
      FAR const int8_t *pa = pool + sc->off_a;
      FAR const int8_t *pb = pool + sc->off_b;
      float    ka = sc->sc_a;
      float    kb = sc->sc_b;
      float    za = (float)sc->zp_a;
      float    zb = (float)sc->zp_b;
      uint32_t i;

      for (i = 0; i < sc->nanchor; i++)
        {
          float va = ((float)pa[i] - za) * ka;
          float vb = ((float)pb[i] - zb) * kb;
          float v;

          /* Both factors are sigmoid outputs and the reference clips them
           * into 0..1 before multiplying - int8 rounding can push a
           * saturated one slightly past it.
           */

          if (va < 0.0f)
            {
              va = 0.0f;
            }
          else if (va > 1.0f)
            {
              va = 1.0f;
            }

          if (vb < 0.0f)
            {
              vb = 0.0f;
            }
          else if (vb > 1.0f)
            {
              vb = 1.0f;
            }

          v = sqrtf(va * vb);

          if (v > best_sc)
            {
              best_sc = v;
              best_s  = (uint32_t)s;
              best_i  = i;
            }
        }
    }

  out->score  = best_sc;
  out->stride = g_face_scale[best_s].stride;
  out->anchor = best_i;

  if (best_sc < EYE_CAM_FACE_THR)
    {
      return 0;
    }

  {
    FAR const struct eye_cam_face_scale_s *sc = &g_face_scale[best_s];
    FAR const int8_t *kps = pool + sc->off_kps + (size_t)best_i * 10;
    uint32_t cols = EYE_CAM_FACE_W / sc->stride;
    float    row  = (float)(best_i / cols);
    float    col  = (float)(best_i % cols);
    float    sx   = (float)EYE_CAM_FRAME_W / (float)EYE_CAM_FACE_W;
    float    sy   = (float)EYE_CAM_FRAME_H / (float)EYE_CAM_FACE_H;
    float    st   = (float)sc->stride;
    float    kq   = sc->sc_kps;
    float    kz   = (float)sc->zp_kps;
    int      k;

    for (k = 0; k < 5; k++)
      {
        float kx = ((float)kps[2 * k] - kz) * kq;
        float ky = ((float)kps[2 * k + 1] - kz) * kq;

        out->kp[2 * k]     = (col + kx) * st * sx;
        out->kp[2 * k + 1] = (row + ky) * st * sy;
      }

    out->ex = (out->kp[0] + out->kp[2]) * 0.5f;
    out->ey = (out->kp[1] + out->kp[3]) * 0.5f;
    out->dx = fabsf(out->kp[2] - out->kp[0]);
  }

  return 1;
}


/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: eye_cam_cap_put_u16 / eye_cam_cap_put_u32
 *
 * Description:
 *   Little endian stores for the BMP header.  BMP is defined that way, so
 *   these stay even on a big endian build.
 *
 ****************************************************************************/

static void eye_cam_cap_put_u16(FAR uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
}

static void eye_cam_cap_put_u32(FAR uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
}

/****************************************************************************
 * Name: eye_cam_cap_header
 *
 * Description:
 *   Write the 14 byte file header and 40 byte DIB header of a 24bpp
 *   uncompressed BMP.  No palette: the images are colour.
 *
 ****************************************************************************/

static void eye_cam_cap_header(FAR FILE *fp, int w, int h, uint32_t imgsz)
{
  FAR uint8_t hdr[54];

  memset(hdr, 0, sizeof(hdr));

  hdr[0] = 'B';
  hdr[1] = 'M';
  eye_cam_cap_put_u32(&hdr[2], 54 + imgsz);       /* file size          */
  eye_cam_cap_put_u32(&hdr[10], 54);              /* pixel data offset  */
  eye_cam_cap_put_u32(&hdr[14], 40);              /* DIB header size    */
  eye_cam_cap_put_u32(&hdr[18], (uint32_t)w);
  eye_cam_cap_put_u32(&hdr[22], (uint32_t)h);
  eye_cam_cap_put_u16(&hdr[26], 1);               /* colour planes      */
  eye_cam_cap_put_u16(&hdr[28], 24);              /* bits per pixel     */
  eye_cam_cap_put_u32(&hdr[34], imgsz);
  eye_cam_cap_put_u32(&hdr[38], 2835);            /* 72 dpi             */
  eye_cam_cap_put_u32(&hdr[42], 2835);
  eye_cam_cap_put_u32(&hdr[46], 0);               /* no palette         */
  eye_cam_cap_put_u32(&hdr[50], 0);

  fwrite(hdr, 1, sizeof(hdr), fp);
}

/****************************************************************************
 * Name: eye_cam_cap_row
 *
 * Description:
 *   Convert one crop row out of the framebuffer, one output pixel per
 *   source pixel.  The crop follows the face, so near the edge of the
 *   frame it hangs over it: outside pixels are black, matching what the
 *   classifier's input gets from the same rectangle.
 *
 *   Channel expansion goes through g_exp5/g_exp6, the tables the input
 *   path uses, so a pixel means the same thing in a captured file and in
 *   a tensor.
 *
 ****************************************************************************/

static void eye_cam_cap_row(FAR const uint16_t *fb, uint32_t stride_px,
                            int rx, int ry, int rw, int y,
                            FAR uint8_t *row)
{
  int sy = ry + y;
  int x;

  if (sy < 0 || sy >= EYE_CAM_FRAME_H)
    {
      memset(row, 0, (size_t)rw * 3);
      return;
    }

  {
    FAR const uint16_t *srow = &fb[(size_t)sy * stride_px];

    for (x = 0; x < rw; x++)
      {
        int sx = rx + x;
        uint16_t px = 0;

        if (sx >= 0 && sx < EYE_CAM_FRAME_W)
          {
            px = srow[sx];
          }

        /* BMP pixel order is B, G, R */

        row[x * 3 + 0] = g_exp5[px & 0x1f];
        row[x * 3 + 1] = g_exp6[(px >> 5) & 0x3f];
        row[x * 3 + 2] = g_exp5[(px >> 11) & 0x1f];
      }
  }
}

/****************************************************************************
 * Name: eye_cam_cap_next_seq
 *
 * Description:
 *   First unused file number for a label.  Probing with fopen rather than
 *   scanning the directory keeps the FATFS work to one entry per frame
 *   already present, and means a second run of the same class adds to the
 *   set instead of overwriting it - a bad batch can be deleted and redone
 *   without touching the good frames.
 *
 ****************************************************************************/

static int eye_cam_cap_next_seq(FAR const char *label)
{
  char path[160];
  FAR FILE *fp;
  int n;

  for (n = 1; n < 10000; n++)
    {
      snprintf(path, sizeof(path), EYE_CAM_CAP_DIR "/%s_%04d.bmp",
               label, n);

      fp = fopen(path, "rb");
      if (fp == NULL)
        {
          return n;
        }

      fclose(fp);
    }

  return -1;
}

/****************************************************************************
 * Name: eye_cam_cap_save
 *
 * Description:
 *   Write one frame's crop.  The image is streamed a row at a time: a
 *   whole 288x139 colour image in RAM would be 120 KB of contiguous heap
 *   for nothing, since the row is all that changes between iterations.
 *
 *   Rows go out bottom up, which is how BMP stores them, and the pad bytes
 *   that bring a row up to a multiple of four are zeroed once rather than
 *   per row - they are the same bytes every time.
 *
 ****************************************************************************/

static int eye_cam_cap_save(FAR const char *label, int seq, uint32_t stride_px,
                            FAR const struct eye_cam_face_s *face,
                            FAR FILE *meta)
{
  char path[160];
  FAR uint8_t *row;
  FAR FILE *fp;
  int rw = g_crop_w;
  int rh = g_crop_h;
  int rowsz;
  int y;

  if (rw <= 0 || rh <= 0 ||
      rw > EYE_CAM_CAP_MAX_W || rh > EYE_CAM_CAP_MAX_H)
    {
      return -EINVAL;
    }

  rowsz = (rw * 3 + 3) & ~3;

  row = malloc((size_t)rowsz);
  if (row == NULL)
    {
      return -ENOMEM;
    }

  memset(row, 0, (size_t)rowsz);

  snprintf(path, sizeof(path), EYE_CAM_CAP_DIR "/%s_%04d.bmp", label, seq);

  fp = fopen(path, "wb");
  if (fp == NULL)
    {
      free(row);
      return -errno;
    }

  eye_cam_cap_header(fp, rw, rh, (uint32_t)rowsz * (uint32_t)rh);

  for (y = rh - 1; y >= 0; y--)
    {
      eye_cam_cap_row(g_fb, stride_px, g_crop_x, g_crop_y, rw, y, row);

      if (fwrite(row, 1, (size_t)rowsz, fp) != (size_t)rowsz)
        {
          break;
        }
    }

  fclose(fp);
  free(row);

  /* One line per frame, so a badly placed crop can be found and dropped
   * on the PC side without opening a single image.  Flushed each time:
   * the card is pulled at the end of a session and an unflushed tail
   * would lose exactly the frames that show where the box drifted.
   */

  if (meta != NULL)
    {
      fprintf(meta, "%s %04d %d %d %d %d %.3f %.1f %.1f %.1f\n",
              label, seq, g_crop_x, g_crop_y, rw, rh,
              (double)face->score, (double)face->ex, (double)face->ey,
              (double)face->dx);
      fflush(meta);
    }

  return 0;
}

/****************************************************************************
 * Name: eye_ui_*  (LVGL eye-control panel)
 *
 * The menu is a grid of square cells rather than a list, because gaze moves
 * in discrete steps: one look-left/right lands exactly on the neighbouring
 * cell.  The classifier runs at a few frames per second, so a six-row list
 * would need five separate dwells to reach the last entry and every dwell
 * is over a second long.
 *
 * This mode owns the panel.  The camera writes the very framebuffer the
 * LTDC scans out (see the file header), so the capture stream has to stay
 * stopped for as long as the grid is up.  Keeping the two exclusive is what
 * makes the single-buffer design safe; a mixed mode would have to freeze
 * the stream first.
 *
 * The layout deliberately avoids rounded corners, shadows, gradients and
 * animations: LVGL runs in PARTIAL mode over a narrow draw buffer here, so
 * every blended pixel costs scanline time the LTDC is waiting on.
 ****************************************************************************/

#if defined(CONFIG_GRAPHICS_LVGL)

/* The fonts are generated offline by gen_eye_font.py and compiled in.  The
 * character set is baked into flash, so a glyph that was never generated
 * renders as nothing at all rather than falling back to a box.
 */

extern const lv_font_t eye_ui_font_48;
extern const lv_font_t eye_ui_font_32;
extern const lv_font_t eye_ui_font_24;
extern const lv_font_t eye_ui_font_64;

#define EYE_UI_COLS          3
#define EYE_UI_ROWS          2
#define EYE_UI_COUNT         (EYE_UI_COLS * EYE_UI_ROWS)
#define EYE_UI_GAP           16
#define EYE_UI_MARGIN_X      30
#define EYE_UI_MARGIN_TOP    58
#define EYE_UI_MARGIN_BOT    46
#define EYE_UI_ICON_TOP      26
#define EYE_UI_LABEL_TOP     112
#define EYE_UI_MIN_IDLE_MS   5

static FAR const char *g_ui_icon[EYE_UI_COUNT] =
{
  "眼", "灯", "拍", "讯", "串", "问"
};

/* Menu names.  Cell 0 was "相机预览" until the recognition overlay turned
 * out to be the more useful description of what it shows.
 */

static FAR const char *g_ui_text[EYE_UI_COUNT] =
{
  "眼动识别展示", "LED 控制", "拍照", "系统信息", "串口命令", "关于"
};

static FAR lv_obj_t *g_ui_cell[EYE_UI_COUNT];
static FAR lv_obj_t *g_ui_icon_lbl[EYE_UI_COUNT];
static FAR lv_obj_t *g_ui_text_lbl[EYE_UI_COUNT];

/* The read-only pages put their content in one wrapped label rather than in
 * cells: cells are buttons, and a page of build and runtime facts is not.
 */

static FAR lv_obj_t *g_ui_text_page;

/* The shot page's countdown.  It sits on top of the camera frame, so it gets
 * its own opaque box to stay readable over whatever the camera is pointed at.
 */

static FAR lv_obj_t *g_ui_badge;
static FAR lv_obj_t *g_ui_badge_lbl;
static FAR lv_obj_t *g_ui_badge_hint;

static int           g_ui_sel;
static volatile bool g_ui_quit;
static FAR lv_obj_t *g_ui_title;

/****************************************************************************
 * Name: eye_ui_highlight
 *
 * Repaint the selection.  Only the two colours change, so LVGL redraws just
 * the affected cells instead of the whole screen.
 ****************************************************************************/

/* The page classes and the body painter are defined with the camera and page
 * text machinery further down, but eye_ui_page_draw() - which is up here with
 * the rest of the page handling - is their first caller.
 */

static bool eye_ui_text_mode(void);
static bool eye_ui_camera_mode(void);
static void eye_ui_body_draw(void);
static void eye_ui_badge_draw(void);
static void eye_ui_photo_retake(void);
static int  eye_ui_photo_save(void);

static void eye_ui_highlight(int sel)
{
  int i;

  for (i = 0; i < EYE_UI_COUNT; i++)
    {
      bool on = (i == sel);

      lv_obj_set_style_bg_color(g_ui_cell[i],
                                on ? lv_color_hex(0x008ca0) :
                                     lv_color_hex(0x262630), 0);
      lv_obj_set_style_border_color(g_ui_cell[i],
                                    on ? lv_color_hex(0x00d2e6) :
                                         lv_color_hex(0x484854), 0);
      lv_obj_set_style_text_color(g_ui_icon_lbl[i],
                                  on ? lv_color_hex(0xffffff) :
                                       lv_color_hex(0xebebf0), 0);
      lv_obj_set_style_text_color(g_ui_text_lbl[i],
                                  on ? lv_color_hex(0xffffff) :
                                       lv_color_hex(0x9696a0), 0);
    }

  g_ui_sel = sel;
}

/****************************************************************************
 * Name: eye_ui_build
 ****************************************************************************/

static void eye_ui_build(void)
{
  FAR lv_obj_t *scr = lv_screen_active();
  FAR lv_obj_t *cell;
  int cw;
  int ch;
  int i;

  lv_obj_set_style_bg_color(scr, lv_color_hex(0x101014), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE);

  cw = (EYE_CAM_FRAME_W - 2 * EYE_UI_MARGIN_X -
        (EYE_UI_COLS - 1) * EYE_UI_GAP) / EYE_UI_COLS;
  ch = (EYE_CAM_FRAME_H - EYE_UI_MARGIN_TOP - EYE_UI_MARGIN_BOT -
        (EYE_UI_ROWS - 1) * EYE_UI_GAP) / EYE_UI_ROWS;

  g_ui_text_page = lv_label_create(scr);
  lv_label_set_text(g_ui_text_page, "");
  lv_obj_set_style_text_font(g_ui_text_page, &eye_ui_font_24, 0);
  lv_obj_set_style_text_color(g_ui_text_page, lv_color_hex(0xd8d8e0), 0);
  lv_label_set_long_mode(g_ui_text_page, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(g_ui_text_page, 800 - 2 * EYE_UI_MARGIN_X);
  lv_obj_align(g_ui_text_page, LV_ALIGN_TOP_LEFT, EYE_UI_MARGIN_X,
               EYE_UI_MARGIN_TOP);
  lv_obj_add_flag(g_ui_text_page, LV_OBJ_FLAG_HIDDEN);

  /* Bottom, not centre: the centre of the frame is the face, and a
   * countdown sitting on the face is no use to somebody trying to pose.
   */

  g_ui_badge = lv_obj_create(scr);
  lv_obj_set_size(g_ui_badge, 220, 160);
  lv_obj_align(g_ui_badge, LV_ALIGN_BOTTOM_MID, 0, -12);
  lv_obj_set_style_radius(g_ui_badge, 16, 0);
  lv_obj_set_style_bg_color(g_ui_badge, lv_color_hex(0x000000), 0);
  lv_obj_set_style_bg_opa(g_ui_badge, LV_OPA_60, 0);
  lv_obj_set_style_border_color(g_ui_badge, lv_color_hex(0x00d2e6), 0);
  lv_obj_set_style_border_width(g_ui_badge, 2, 0);
  lv_obj_set_style_pad_all(g_ui_badge, 0, 0);
  lv_obj_clear_flag(g_ui_badge, LV_OBJ_FLAG_SCROLLABLE);

  g_ui_badge_lbl = lv_label_create(g_ui_badge);
  lv_label_set_text(g_ui_badge_lbl, "3");
  lv_obj_set_style_text_font(g_ui_badge_lbl, &eye_ui_font_64, 0);
  lv_obj_set_style_text_color(g_ui_badge_lbl, lv_color_hex(0xffffff), 0);
  lv_obj_align(g_ui_badge_lbl, LV_ALIGN_TOP_MID, 0, 0);

  g_ui_badge_hint = lv_label_create(g_ui_badge);
  lv_label_set_text(g_ui_badge_hint, "");
  lv_obj_set_style_text_font(g_ui_badge_hint, &eye_ui_font_24, 0);
  lv_obj_set_style_text_color(g_ui_badge_hint, lv_color_hex(0xa0e0ec), 0);
  lv_obj_align(g_ui_badge_hint, LV_ALIGN_BOTTOM_MID, 0, 4);

  lv_obj_add_flag(g_ui_badge, LV_OBJ_FLAG_HIDDEN);

  g_ui_title = lv_label_create(scr);
  lv_label_set_text(g_ui_title, "眼控面板");
  lv_obj_set_style_text_font(g_ui_title, &eye_ui_font_32, 0);
  lv_obj_set_style_text_color(g_ui_title, lv_color_hex(0xebebf0), 0);
  lv_obj_align(g_ui_title, LV_ALIGN_TOP_MID, 0, 4);

  for (i = 0; i < EYE_UI_COUNT; i++)
    {
      int r = i / EYE_UI_COLS;
      int c = i % EYE_UI_COLS;

      cell = lv_obj_create(scr);
      lv_obj_set_pos(cell, EYE_UI_MARGIN_X + c * (cw + EYE_UI_GAP),
                     EYE_UI_MARGIN_TOP + r * (ch + EYE_UI_GAP));
      lv_obj_set_size(cell, cw, ch);
      lv_obj_set_style_radius(cell, 0, 0);
      lv_obj_set_style_pad_all(cell, 0, 0);
      lv_obj_set_style_border_width(cell, 1, 0);
      lv_obj_set_style_bg_opa(cell, LV_OPA_COVER, 0);
      lv_obj_clear_flag(cell, LV_OBJ_FLAG_SCROLLABLE);
      g_ui_cell[i] = cell;

      g_ui_icon_lbl[i] = lv_label_create(cell);
      lv_label_set_text(g_ui_icon_lbl[i], g_ui_icon[i]);
      lv_obj_set_style_text_font(g_ui_icon_lbl[i], &eye_ui_font_48, 0);
      lv_obj_align(g_ui_icon_lbl[i], LV_ALIGN_TOP_MID, 0, EYE_UI_ICON_TOP);

      g_ui_text_lbl[i] = lv_label_create(cell);
      lv_label_set_text(g_ui_text_lbl[i], g_ui_text[i]);
      lv_obj_set_style_text_font(g_ui_text_lbl[i], &eye_ui_font_24, 0);
      lv_obj_align(g_ui_text_lbl[i], LV_ALIGN_TOP_MID, 0, EYE_UI_LABEL_TOP);
    }

  eye_ui_highlight(0);
}

/****************************************************************************
 * Name: eye_ui_run
 *
 * Owns the LVGL lifecycle for the whole panel.  Returns only when the panel
 * is asked to quit, which nothing does yet - the hook is here because mode
 * switching will need it (the camera cannot start until the panel handler
 * and its framebuffer are released).
 ****************************************************************************/

/****************************************************************************
 * Name: eye_cam_xspi_probe
 *
 * Description:
 *   Walk the HyperRAM window (XSPI1 at 0x90000000) one access at a time,
 *   logging before every step so a fault identifies exactly which access
 *   failed.  Run as "eye_cam xspitest".
 *
 *   The order matters.  A load is a precise bus cycle: the CPU waits for the
 *   data, so a dead window raises CFSR.PRECISERR with a valid BFAR naming the
 *   address.  A store is posted through the AXI write buffer and only reports
 *   later, as CFSR.IMPRECISERR with a meaningless BFAR - which is exactly the
 *   shape of the fault seen when the LVGL pool initialisation stores into
 *   this window.  Testing loads first therefore separates "the window does
 *   not decode at all" from "the window decodes but cannot accept stores".
 *
 ****************************************************************************/

/* XSPI1 register file of STM32N647 (AHB5PERIPH_BASE_NS + 0x5000) */

#define EYE_CAM_XSPI1_BASE   0x48025000u

#define EYE_CAM_XSPI_CR      0x000
#define EYE_CAM_XSPI_SR      0x020
#define EYE_CAM_XSPI_FCR     0x024
#define EYE_CAM_XSPI_HLCR    0x200
#define EYE_CAM_XSPI_WCCR    0x180
#define EYE_CAM_XSPI_WTCR    0x188

static inline uint32_t eye_cam_xspi_get(uint32_t off)
{
  return *(FAR volatile uint32_t *)(uintptr_t)(EYE_CAM_XSPI1_BASE + off);
}

static inline void eye_cam_xspi_put(uint32_t off, uint32_t val)
{
  *(FAR volatile uint32_t *)(uintptr_t)(EYE_CAM_XSPI1_BASE + off) = val;
}

static int eye_cam_xspi_dump(void)
{
  static const struct
  {
    uint32_t            off;
    FAR const char     *name;
  } regs[] =
  {
    { 0x000, "CR"    }, { 0x008, "DCR1"  }, { 0x00c, "DCR2"  },
    { 0x010, "DCR3"  }, { 0x014, "DCR4"  }, { 0x020, "SR"    },
    { 0x024, "FCR"   }, { 0x040, "DLR"   }, { 0x048, "AR"    },
    { 0x050, "DR"    }, { 0x100, "CCR"   }, { 0x108, "TCR"   },
    { 0x110, "IR"    }, { 0x120, "ABR"   }, { 0x130, "LPTR"  },
    { 0x140, "WPCCR" }, { 0x148, "WPTCR" }, { 0x150, "WPIR"  },
    { 0x160, "WPABR" }, { 0x180, "WCCR"  }, { 0x188, "WTCR"  },
    { 0x190, "WIR"   }, { 0x1a0, "WABR"  }, { 0x200, "HLCR"  },
  };

  int i;

  for (i = 0; i < (int)(sizeof(regs) / sizeof(regs[0])); i++)
    {
      syslog(LOG_INFO, "xspi1[%03lx] %-6s = 0x%08lx\n",
             (unsigned long)regs[i].off, regs[i].name,
             (unsigned long)eye_cam_xspi_get(regs[i].off));
    }

  return 0;
}

static int eye_cam_xspi_probe(void);

static int eye_cam_xspi_cmd(int argc, FAR char *argv[])
{
  FAR const char *sub = argv[2];
  uint32_t off;
  uint32_t val;
  uintptr_t addr;

  if (strcmp(sub, "dump") == 0)
    {
      return eye_cam_xspi_dump();
    }

  if (strcmp(sub, "seq") == 0)
    {
      return eye_cam_xspi_probe();
    }

  if (strcmp(sub, "get") == 0 && argc >= 4)
    {
      off = (uint32_t)strtoul(argv[3], NULL, 0);
      syslog(LOG_INFO, "xspi1[%03lx] = 0x%08lx\n", (unsigned long)off,
             (unsigned long)eye_cam_xspi_get(off));
      return 0;
    }

  if (strcmp(sub, "set") == 0 && argc >= 5)
    {
      off = (uint32_t)strtoul(argv[3], NULL, 0);
      val = (uint32_t)strtoul(argv[4], NULL, 0);
      syslog(LOG_INFO, "xspi1[%03lx]: 0x%08lx -> writing 0x%08lx\n",
             (unsigned long)off, (unsigned long)eye_cam_xspi_get(off),
             (unsigned long)val);

      eye_cam_xspi_put(off, val);

      syslog(LOG_INFO, "xspi1[%03lx]: reads back 0x%08lx\n",
             (unsigned long)off, (unsigned long)eye_cam_xspi_get(off));
      return 0;
    }

  if ((strcmp(sub, "rd") == 0 || strcmp(sub, "rd16") == 0 ||
       strcmp(sub, "rd8") == 0) && argc >= 4)
    {
      addr = (uintptr_t)strtoul(argv[3], NULL, 0);

      syslog(LOG_INFO, "load  %s  0x%08lx\n", sub, (unsigned long)addr);

      if (strcmp(sub, "rd8") == 0)
        {
          val = *(FAR volatile uint8_t *)addr;
        }
      else if (strcmp(sub, "rd16") == 0)
        {
          val = *(FAR volatile uint16_t *)addr;
        }
      else
        {
          val = *(FAR volatile uint32_t *)addr;
        }

      syslog(LOG_INFO, "load  ok, value 0x%08lx\n", (unsigned long)val);
      return 0;
    }

  if ((strcmp(sub, "wr") == 0 || strcmp(sub, "wr16") == 0 ||
       strcmp(sub, "wr8") == 0) && argc >= 5)
    {
      addr = (uintptr_t)strtoul(argv[3], NULL, 0);
      val  = (uint32_t)strtoul(argv[4], NULL, 0);

      syslog(LOG_INFO, "store %s  0x%08lx = 0x%08lx\n", sub,
             (unsigned long)addr, (unsigned long)val);

      if (strcmp(sub, "wr8") == 0)
        {
          *(FAR volatile uint8_t *)addr = (uint8_t)val;
        }
      else if (strcmp(sub, "wr16") == 0)
        {
          *(FAR volatile uint16_t *)addr = (uint16_t)val;
        }
      else
        {
          *(FAR volatile uint32_t *)addr = val;
        }

      /* The store itself is posted; nothing has completed yet.  Any of the
       * syslog() calls below - or the load back - is what forces the AXI
       * write buffer to drain, and that is where an imprecise bus fault
       * would surface.
       */

      syslog(LOG_INFO, "store posted; draining write buffer\n");
      syslog(LOG_INFO, "read back 0x%08lx = 0x%08lx\n", (unsigned long)addr,
             (unsigned long)*(FAR volatile uint32_t *)addr);
      return 0;
    }

  syslog(LOG_INFO, "usage: eye_cam xspi dump|seq|get|set|rd|rd8|rd16|wr|wr8|wr16\n");
  return 0;
}

static int eye_cam_xspi_probe(void)
{
  FAR volatile uint32_t *base = (FAR volatile uint32_t *)0x90000000;
  uint32_t val;

  syslog(LOG_INFO, "xspi-probe: step 1 - load  0x90000000\n");
  val = base[0];
  syslog(LOG_INFO, "xspi-probe: step 1 ok, value 0x%08lx\n",
         (unsigned long)val);

  syslog(LOG_INFO, "xspi-probe: step 2 - load  0x90000004\n");
  val = base[1];
  syslog(LOG_INFO, "xspi-probe: step 2 ok, value 0x%08lx\n",
         (unsigned long)val);

  syslog(LOG_INFO, "xspi-probe: step 3 - load  0x90040000 (256 KB in)\n");
  val = base[0x10000];
  syslog(LOG_INFO, "xspi-probe: step 3 ok, value 0x%08lx\n",
         (unsigned long)val);

  syslog(LOG_INFO, "xspi-probe: step 4 - store 0x90000000 = 0xa5a5a5a5\n");
  base[0] = 0xa5a5a5a5;
  syslog(LOG_INFO, "xspi-probe: step 4 posted\n");

  syslog(LOG_INFO, "xspi-probe: step 5 - drain + load back\n");
  val = base[0];
  syslog(LOG_INFO, "xspi-probe: step 5 value 0x%08lx (want 0xa5a5a5a5)\n",
         (unsigned long)val);

  syslog(LOG_INFO, "xspi-probe: done\n");
  return 0;
}

/* The capture destination is owned by the STM32N6 video driver.  Declared
 * here rather than included: an application has no business pulling in an
 * architecture header.
 */

extern void stm32n6_video_set_capture_dest(uint32_t addr);
extern uint32_t stm32n6_video_capture_dest(void);

/* --- Step 2: eye verdict -> panel ---------------------------------------- *
 *
 * Hold times in milliseconds.  The loop runs at whatever rate the capture
 * pipeline manages, so the hold has to be measured in time - a frame count
 * would mean a different glance duration every time the load changes.
 */

#define EYE_UI_HOLD_NAV   1200   /* look left/right this long to step */
#define EYE_UI_HOLD_OK     800   /* close your eyes this long to confirm */
#define EYE_UI_HOLD_BACK  2000   /* on a page: look left this long to leave */
#define EYE_UI_COOLDOWN    600   /* ignore everything right after a trigger */

/* Where captured frames land in UI mode.  It has to stay clear of the LVGL
 * pool, which lives at 0x90000000 and is CONFIG_LV_MEM_SIZE_KILOBYTES long:
 * one frame is 800x480x2 = 768000 bytes, so 1 MB in is comfortably clear.
 */

#define EYE_UI_CAM_ADDR  0x90100000u

static bool    g_ui_mode;
static int     g_ui_last = -1;
static int64_t g_ui_since;
static bool    g_ui_tracking;
static bool    g_ui_armed = true;
static int64_t g_ui_cooldown_until;
static lv_nuttx_result_t g_ui_result;

static int64_t eye_ui_ms(void)
{
  struct timespec ts;

  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (int64_t)ts.tv_sec * 1000 + (int64_t)(ts.tv_nsec / 1000000);
}

static int eye_ui_lvgl_start(void)
{
  lv_nuttx_dsc_t info;

  lv_init();
  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif

  lv_nuttx_init(&info, &g_ui_result);

  if (g_ui_result.disp == NULL)
    {
      return -1;
    }

  eye_ui_build();

  eye_cam_out("eye_ui: panel up, %dx%d cells, select=%d\n",
              EYE_UI_COLS, EYE_UI_ROWS, g_ui_sel);
  return 0;
}

static void eye_ui_lvgl_stop(void)
{
  lv_nuttx_deinit(&g_ui_result);
  lv_deinit();
}

/* --- second-level pages -------------------------------------------------- *
 *
 * A page reuses the menu's cells: fewer of them, no icon, text centred.  That
 * way there is one highlight implementation and one selection index for all
 * three pages, and the panel never has to be rebuilt.
 */

typedef enum
{
  EYE_UI_PAGE_GRID = 0,   /* the 3x2 menu */
  EYE_UI_PAGE_LED,        /* LED control */
  EYE_UI_PAGE_INFO,       /* system information */
  EYE_UI_PAGE_ABOUT,      /* what this build is */
  EYE_UI_PAGE_CMD,        /* the console commands it answers to */
  EYE_UI_PAGE_PHOTO,      /* countdown, then a shot */
  EYE_UI_PAGE_PREVIEW,    /* live camera, and no chrome at all */
} eye_ui_page_t;

#define EYE_UI_LED_CELLS    3    /* LED 0, LED 1, back */
#define EYE_UI_INFO_CELLS   5    /* four readings and back */
#define EYE_UI_ABOUT_CELLS  5    /* four facts and back */
#define EYE_UI_CMD_CELLS    4    /* three commands and back */

/* The shot is taken three seconds after the blink that asked for it, so there
 * is time to get back into position.
 */

#define EYE_UI_PHOTO_DELAY  3000
#define EYE_UI_FONT64_CHARS "0123456789OKFAIL"

#define EYE_UI_PHOTO_IDLE   0
#define EYE_UI_PHOTO_WAIT   1
#define EYE_UI_PHOTO_TAKEN  2
#define EYE_UI_PHOTO_FAILED 3

static int      g_ui_page  = EYE_UI_PAGE_GRID;
static int      g_ui_cells = EYE_UI_COUNT;
static int      g_led_fd   = -1;
static uint32_t g_led_state;
static int      g_ui_fps10;
static int64_t  g_ui_prev_ms;

/* Where the panel's own framebuffer is.  g_fb is re-pointed at the capture
 * buffer in UI mode, so the preview page needs the original address kept.
 */

static FAR uint16_t *g_lcd_fb;
static uint32_t      g_lcd_stride_px;

/* Shot state.  The countdown runs off the monotonic clock rather than frame
 * counts, so it means the same three seconds whatever the frame rate.
 */

static int      g_photo_state;
static int      g_photo_shown;
static int      g_photo_seq;
static int64_t  g_photo_until;

/* Last things the capture loop saw, for the live page. */

static int      g_ui_frames;
static int      g_ui_last_cls = -1;
static int      g_ui_last_conf;
static float    g_ui_face_score;
static bool     g_ui_face_ok;

/* Read the LED state back rather than tracking it: the LEDs are shared with
 * whatever else is on the board, so the driver's view is the only truthful
 * one.
 */

static void eye_ui_led_sync(void)
{
  uint32_t all = 0;

  if (g_led_fd < 0)
    {
      g_led_fd = open("/dev/userleds", O_WRONLY);
      if (g_led_fd < 0)
        {
          eye_cam_out("eye_ui: open /dev/userleds failed: %d\n", errno);
          return;
        }
    }

  if (ioctl(g_led_fd, ULEDIOC_GETALL, (unsigned long)(uintptr_t)&all) == 0)
    {
      g_led_state = all;
    }
}

static bool eye_ui_led_toggle(int idx)
{
  struct userled_s led;

  if (g_led_fd < 0)
    {
      eye_ui_led_sync();
      if (g_led_fd < 0)
        {
          return false;
        }
    }

  led.ul_led = (uint8_t)idx;
  led.ul_on  = (g_led_state & (1u << idx)) == 0;

  if (ioctl(g_led_fd, ULEDIOC_SETLED, (unsigned long)(uintptr_t)&led) < 0)
    {
      eye_cam_out("eye_ui: LED %d set failed: %d\n", idx, errno);
      return false;
    }

  if (led.ul_on)
    {
      g_led_state |= (1u << idx);
    }
  else
    {
      g_led_state &= ~(1u << idx);
    }

  return true;
}

/* The label for cell i of the page we are on.  The menu has static names; the
 * other two pages fill in a live value, which is why this hands back a pointer
 * into a scratch buffer for those.
 */

static FAR const char *eye_ui_text_at(int i)
{
  static char buf[48];
  struct mallinfo mem;

  if (g_ui_page == EYE_UI_PAGE_LED)
    {
      if (i >= EYE_UI_LED_CELLS - 1)
        {
          return "返回";
        }

      snprintf(buf, sizeof(buf), "LED %d 已%s", i,
               (g_led_state & (1u << i)) ? "亮" : "灭");
      return buf;
    }

  if (g_ui_page == EYE_UI_PAGE_ABOUT)
    {
      switch (i)
        {
          case 0:
            return "眼控交互";

          case 1:
            return "人脸 YuNet";

          case 2:
            return "眼动 五分类";

          case 3:
            return "NPU 600 GOPS";

          default:
            return "返回";
        }
    }

  if (g_ui_page == EYE_UI_PAGE_CMD)
    {
      if (i >= EYE_UI_CMD_CELLS - 1)
        {
          return "返回";
        }

      /* ASCII on this page is deliberate: the cells are things to type, and
       * the 24 px tier carries ASCII precisely so they can be shown.
       */

      switch (i)
        {
          case 0:
            return "eye_cam 100 500";

          case 1:
            return "eye_cam panel";

          default:
            return "eye_cam xspi dump";
        }
    }

  if (g_ui_page == EYE_UI_PAGE_INFO)
    {
      switch (i)
        {
          case 0:
            snprintf(buf, sizeof(buf), "版本 %s", __DATE__);
            return buf;

          case 1:
            mem = mallinfo();
            if (mem.fordblks > 1024 * 1024)
              {
                snprintf(buf, sizeof(buf), "内存 %d M", mem.fordblks >> 20);
              }
            else
              {
                snprintf(buf, sizeof(buf), "内存 %d K", mem.fordblks >> 10);
              }
            return buf;

          case 2:
            snprintf(buf, sizeof(buf), "帧率 %d.%d", g_ui_fps10 / 10,
                     g_ui_fps10 % 10);
            return buf;

          case 3:
            snprintf(buf, sizeof(buf), "时间 %d 分",
                     (int)(eye_ui_ms() / 60000));
            return buf;

          default:
            return "返回";
        }
    }

  return g_ui_text[i];
}

/* Paint the current page's cells.  Selection is deliberately left alone, so
 * this doubles as the refresh action.
 */

static void eye_ui_page_draw(void)
{
  int i;

  eye_ui_body_draw();

  for (i = 0; i < EYE_UI_COUNT; i++)
    {
      if (eye_ui_camera_mode() || eye_ui_text_mode() || i >= g_ui_cells)
        {
          continue;
        }

      if (g_ui_page == EYE_UI_PAGE_GRID)
        {
          lv_label_set_text(g_ui_icon_lbl[i], g_ui_icon[i]);
          lv_obj_clear_flag(g_ui_icon_lbl[i], LV_OBJ_FLAG_HIDDEN);
          lv_label_set_text(g_ui_text_lbl[i], g_ui_text[i]);
          lv_obj_align(g_ui_text_lbl[i], LV_ALIGN_TOP_MID, 0,
                       EYE_UI_LABEL_TOP);
        }
      else
        {
          lv_obj_add_flag(g_ui_icon_lbl[i], LV_OBJ_FLAG_HIDDEN);
          lv_label_set_text(g_ui_text_lbl[i], eye_ui_text_at(i));
          lv_obj_align(g_ui_text_lbl[i], LV_ALIGN_CENTER, 0, 0);
        }
    }
}

/* Switch pages, and start at the first cell. */

static void eye_ui_page_enter(int page)
{
  bool was_camera = eye_ui_camera_mode();

  g_ui_page = page;
  g_ui_sel  = 0;

  switch (page)
    {
      case EYE_UI_PAGE_LED:
        g_ui_cells = EYE_UI_LED_CELLS;
        eye_ui_led_sync();
        lv_label_set_text(g_ui_title, "控制");
        break;

      case EYE_UI_PAGE_INFO:
        g_ui_cells = EYE_UI_INFO_CELLS;
        lv_label_set_text(g_ui_title, "系统信息");
        break;

      case EYE_UI_PAGE_ABOUT:
        g_ui_cells = EYE_UI_ABOUT_CELLS;
        lv_label_set_text(g_ui_title, "关于");
        break;

      case EYE_UI_PAGE_CMD:
        g_ui_cells = EYE_UI_CMD_CELLS;
        lv_label_set_text(g_ui_title, "串口命令");
        break;

      case EYE_UI_PAGE_PHOTO:
        g_ui_cells    = 0;      /* the camera frame is the whole page */
        g_photo_state = EYE_UI_PHOTO_WAIT;
        g_photo_shown = EYE_UI_PHOTO_DELAY / 1000;
        g_photo_until = eye_ui_ms() + EYE_UI_PHOTO_DELAY;
        break;

      case EYE_UI_PAGE_PREVIEW:
        g_ui_cells = 0;
        break;

      default:
        g_ui_cells = EYE_UI_COUNT;
        lv_label_set_text(g_ui_title, "眼控面板");
        break;
    }

  eye_ui_page_draw();

  /* Leaving the shot page cancels a countdown that is still running. */

  if (page != EYE_UI_PAGE_PHOTO)
    {
      g_photo_state = EYE_UI_PHOTO_IDLE;
    }

  if (g_ui_cells > 0)
    {
      eye_ui_highlight(0);
    }

  /* Leaving a camera-backed page, the panel is holding a camera frame rather
   * than anything LVGL drew.  A partial refresh would only repaint the areas
   * LVGL believes changed, so the whole screen has to be invalidated.
   */

  if (was_camera && page != EYE_UI_PAGE_PREVIEW && page != EYE_UI_PAGE_PHOTO)
    {
      lv_obj_invalidate(lv_screen_active());
    }

  eye_cam_out("eye_ui: page %d, %d cells\n", (int)page, g_ui_cells);
}

/* --- the read-only pages ------------------------------------------------- */

/* A page of facts, one per line.  These are ASCII on purpose: the content is
 * build and runtime data, the 24 px tier already carries ASCII, and keeping
 * the format simple means a new fact costs no glyphs.
 *
 * Everything here is either a compile-time constant or read straight from
 * the subsystem that owns it, so the page cannot drift away from the truth
 * the way a hand-maintained table would.
 */

static FAR const char *eye_ui_page_text(void)
{
  static char buf[1024];
  lv_mem_monitor_t mon;
  struct mallinfo   mem;
  char *p = buf;
  size_t room;

  if (g_ui_page == EYE_UI_PAGE_ABOUT)
    {
      snprintf(buf, sizeof(buf),
               "eye_cam  eye controlled panel\n"
               "built    %s %s\n"
               "board    ATK-DNN647 / STM32N647X0H3Q\n"
               "cpu      Cortex-M55 at 800 MHz\n"
               "npu      Neural-ART, ATON runtime\n"
               "os       openvela / NuttX %s (%s)\n"
               "gui      LVGL %d.%d.%d\n"
               "face     YuNet 256x416 int8\n"
               "gaze     5 class CNN int8\n"
               "targets  xSPI2 NOR  weight blob\n"
               "ram      internal + 32 MB xSPI1\n",
               __DATE__, __TIME__,
               CONFIG_VERSION_STRING, CONFIG_VERSION_BUILD,
               LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
      return buf;
    }

  /* Live figures.  The LVGL heap is worth showing because it lives in the
   * HyperRAM: a sane number there is also the cheapest proof that the
   * external memory is still working.
   */

  mem = mallinfo();
  lv_mem_monitor(&mon);

  room = sizeof(buf);

#define EYE_UI_ADD(...) \
  do { \
      int wrote = snprintf(p, room, __VA_ARGS__); \
      if (wrote > 0 && (size_t)wrote < room) \
        { \
          p    += wrote; \
          room -= (size_t)wrote; \
        } \
    } while (0)

  EYE_UI_ADD("uptime   %d min %d s\n",
             (int)(eye_ui_ms() / 60000), (int)((eye_ui_ms() / 1000) % 60));
  EYE_UI_ADD("frames   %d\n", g_ui_frames);
  EYE_UI_ADD("fps      %d.%d\n", g_ui_fps10 / 10, g_ui_fps10 % 10);
  EYE_UI_ADD("verdict  %s  %d%%\n",
             g_ui_last_cls >= 0 ? g_eye_cam_name[g_ui_last_cls] : "waiting",
             g_ui_last_conf);
  EYE_UI_ADD("face     %s (%.3f)\n",
             g_ui_face_ok ? "found" : "none",
             (double)g_ui_face_score);
  EYE_UI_ADD("heap     %d K free\n", mem.fordblks / 1024);
  EYE_UI_ADD("lvgl     %d of %d K used (%d%%)\n",
             (int)(mon.total_size - mon.free_size) / 1024,
             (int)mon.total_size / 1024, mon.used_pct);
  EYE_UI_ADD("frag     %d%%\n", mon.frag_pct);
  EYE_UI_ADD("capture  0x%08lx\n",
             (unsigned long)stm32n6_video_capture_dest());
  EYE_UI_ADD("panel    /dev/lcd0  800x480\n");

#undef EYE_UI_ADD

  return buf;
}

/* Paint the page's chosen body: cells, text or nothing at all. */

static void eye_ui_body_draw(void)
{
  bool text = eye_ui_text_mode();
  bool cam  = eye_ui_camera_mode();
  int  i;

  if (text)
    {
      lv_label_set_text(g_ui_text_page, eye_ui_page_text());
      lv_obj_clear_flag(g_ui_text_page, LV_OBJ_FLAG_HIDDEN);
    }
  else
    {
      lv_obj_add_flag(g_ui_text_page, LV_OBJ_FLAG_HIDDEN);
    }

  /* The camera pages have neither chrome nor cells: the picture is the page.
   */

  if (cam)
    {
      lv_obj_add_flag(g_ui_title, LV_OBJ_FLAG_HIDDEN);
    }
  else
    {
      lv_obj_clear_flag(g_ui_title, LV_OBJ_FLAG_HIDDEN);
    }

  /* The countdown badge belongs to the shot page and is shown by the camera
   * loop, so every other page has to take it down - nothing else will.
   */

  if (g_ui_page != EYE_UI_PAGE_PHOTO)
    {
      lv_obj_add_flag(g_ui_badge, LV_OBJ_FLAG_HIDDEN);
    }

  for (i = 0; i < EYE_UI_COUNT; i++)
    {
      if (cam || text || i >= g_ui_cells)
        {
          lv_obj_add_flag(g_ui_cell[i], LV_OBJ_FLAG_HIDDEN);
          continue;
        }

      lv_obj_clear_flag(g_ui_cell[i], LV_OBJ_FLAG_HIDDEN);
    }
}

/* Drive the countdown badge.
 *
 * Redrawn every frame rather than only when its text changes: the blit that
 * puts the camera frame down erases it, so "still correct" is not the same
 * as "still on screen".
 */

static void eye_ui_badge_draw(void)
{
  static char buf[16];
  FAR const char *hint;

  if (g_ui_page != EYE_UI_PAGE_PHOTO)
    {
      lv_obj_add_flag(g_ui_badge, LV_OBJ_FLAG_HIDDEN);
      return;
    }

  hint = "";

  switch (g_photo_state)
    {
      case EYE_UI_PHOTO_WAIT:
        snprintf(buf, sizeof(buf), "%d", g_photo_shown);
        break;

      case EYE_UI_PHOTO_TAKEN:
        snprintf(buf, sizeof(buf), "OK");
        hint = "blink to retake";
        break;

      case EYE_UI_PHOTO_FAILED:
        snprintf(buf, sizeof(buf), "FAIL");
        hint = "blink to retake";
        break;

      default:
        snprintf(buf, sizeof(buf), "OK");
        break;
    }

  lv_label_set_text(g_ui_badge_lbl, buf);
  lv_obj_align(g_ui_badge_lbl, LV_ALIGN_TOP_MID, 0, 0);
  lv_label_set_text(g_ui_badge_hint, hint);
  lv_obj_align(g_ui_badge_hint, LV_ALIGN_BOTTOM_MID, 0, 4);

  lv_obj_clear_flag(g_ui_badge, LV_OBJ_FLAG_HIDDEN);
  lv_obj_invalidate(g_ui_badge);
}

/* --- preview, shot, countdown ------------------------------------------- */

/* Pages whose body is a block of text rather than cells. */

static bool eye_ui_text_mode(void)
{
  return g_ui_page == EYE_UI_PAGE_INFO || g_ui_page == EYE_UI_PAGE_ABOUT;
}

/* Pages where the camera owns the panel.  On these LVGL must not flush from
 * the verdict path: the frame is written at the tail of the loop and the
 * badge is drawn over it.
 */

static bool eye_ui_camera_mode(void)
{
  return g_ui_page == EYE_UI_PAGE_PREVIEW || g_ui_page == EYE_UI_PAGE_PHOTO;
}

/* Put the frame the camera just filled on the panel.
 *
 * The two buffers are both 800 wide but they are separate allocations, so
 * copy row by row: a stride difference would shear the picture, and a stride
 * difference is exactly the kind of thing that changes when the panel
 * configuration does.
 *
 * The camera filled the source by DMA, so drop whatever the CPU still has
 * cached for it first.  The copy then has to be pushed back out, because the
 * LTDC scans the panel's buffer from memory.
 */

static void eye_ui_preview_blit(void)
{
  uintptr_t src_end;
  uintptr_t dst_end;
  uint32_t  y;

  if (g_lcd_fb == NULL || g_fb == NULL || !eye_ui_camera_mode())
    {
      return;
    }

  src_end = (uintptr_t)g_fb + (size_t)EYE_CAM_FRAME_H * EYE_CAM_FRAME_W *
                              sizeof(uint16_t);
  dst_end = (uintptr_t)g_lcd_fb + (size_t)EYE_CAM_FRAME_H * g_lcd_stride_px *
                                  sizeof(uint16_t);

  up_invalidate_dcache((uintptr_t)g_fb, src_end);

  for (y = 0; y < EYE_CAM_FRAME_H; y++)
    {
      memcpy(&g_lcd_fb[(size_t)y * g_lcd_stride_px],
             &g_fb[(size_t)y * EYE_CAM_FRAME_W],
             EYE_CAM_FRAME_W * sizeof(uint16_t));
    }

  up_clean_dcache((uintptr_t)g_lcd_fb, dst_end);
}

/* Ask for another shot.  Only meaningful once one has been taken - while a
 * countdown is running the right answer is to let it finish.
 */

static void eye_ui_photo_retake(void)
{
  if (g_photo_state == EYE_UI_PHOTO_WAIT)
    {
      return;
    }

  g_photo_state = EYE_UI_PHOTO_WAIT;
  g_photo_shown = EYE_UI_PHOTO_DELAY / 1000;
  g_photo_until = eye_ui_ms() + EYE_UI_PHOTO_DELAY;

  eye_cam_out("eye_ui: another shot, %d s\n", g_photo_shown);
}

/* Take the shot if its time has come.
 *
 * Called from the capture loop *before* the overlay is drawn.  That ordering
 * is the whole trick: the buffer at this point holds what the camera put
 * there, so the file gets the picture rather than the picture with a
 * detection box and two pupil markers drawn on it.  The frame is stable
 * because the stream is stopped between iterations.
 */

static void eye_ui_photo_maybe_save(void)
{
  if (g_ui_page != EYE_UI_PAGE_PHOTO ||
      g_photo_state != EYE_UI_PHOTO_WAIT)
    {
      return;
    }

  if (eye_ui_ms() < g_photo_until)
    {
      return;
    }

  g_photo_seq   = eye_ui_photo_save();
  g_photo_state = g_photo_seq > 0 ? EYE_UI_PHOTO_TAKEN : EYE_UI_PHOTO_FAILED;

  eye_cam_out("eye_ui: photo %s (%d)\n",
              g_photo_state == EYE_UI_PHOTO_TAKEN ? "saved" : "failed",
              g_photo_seq);
}

/* Save the whole frame as a 24bpp BMP.
 *
 * The capture path's row converter already takes the rectangle it should
 * read as arguments, so a full frame is the same call with a rectangle that
 * happens to be the whole thing - no second converter to keep in step.
 *
 * Returns the file number, or a negative errno.
 */

static int eye_ui_photo_save(void)
{
  char path[96];
  FAR uint8_t *row;
  FAR FILE *fp;
  int rowsz = (EYE_CAM_FRAME_W * 3 + 3) & ~3;
  int y;
  int n;

  mkdir(EYE_UI_PHOTO_DIR, 0777);

  /* Same probe-the-name trick the capture path uses: one open per shot that
   * is already there, and none at all on a fresh card.
   */

  for (n = 1; n < 10000; n++)
    {
      snprintf(path, sizeof(path), EYE_UI_PHOTO_DIR "/photo_%04d.bmp", n);

      fp = fopen(path, "rb");
      if (fp == NULL)
        {
          break;
        }

      fclose(fp);
    }

  if (n >= 10000)
    {
      return -ENOSPC;
    }

  row = malloc((size_t)rowsz);
  if (row == NULL)
    {
      return -ENOMEM;
    }

  memset(row, 0, (size_t)rowsz);

  fp = fopen(path, "wb");
  if (fp == NULL)
    {
      free(row);
      return -errno;
    }

  eye_cam_cap_header(fp, EYE_CAM_FRAME_W, EYE_CAM_FRAME_H,
                     (uint32_t)rowsz * EYE_CAM_FRAME_H);

  /* Bottom row first: that is the order BMP stores rows in.  Everything the
   * overlay drew was pushed out already, so dropping the CPU's copy of the
   * frame now cannot lose a stroke of it.
   */

  up_invalidate_dcache((uintptr_t)g_fb,
                       (uintptr_t)g_fb + (size_t)EYE_CAM_FRAME_H *
                                        EYE_CAM_FRAME_W * sizeof(uint16_t));

  for (y = EYE_CAM_FRAME_H - 1; y >= 0; y--)
    {
      eye_cam_cap_row(g_fb, EYE_CAM_FRAME_W, 0, 0, EYE_CAM_FRAME_W, y, row);

      if (fwrite(row, 1, (size_t)rowsz, fp) != (size_t)rowsz)
        {
          break;
        }
    }

  fclose(fp);
  free(row);

  return n;
}

/* The console explanation for a command cell.  These are the commands the
 * running process will not let you type, because it is the thing holding
 * the camera: they are worth having in front of you while it runs.
 */

static void eye_ui_cmd_explain(int idx)
{
  switch (idx)
    {
      case 0:
        eye_cam_out("eye_cam: 相机模式 - 跑 100 帧、帧间 500 ms，"
                    "屏幕上直接看到带判定框的实时画面\n");
        break;

      case 1:
        eye_cam_out("eye_cam: 纯面板模式 - 不开相机。界面起不来时用它"
                    "区分是 UI 的问题还是采集的问题\n");
        break;

      default:
        eye_cam_out("eye_cam: HyperRAM 实验台 - dump XSPI1 寄存器，"
                    "排查 0x90000000 窗口时用\n");
        break;
    }
}

/* Called once per frame from the capture loop, after the overlay has been
 * drawn and pushed out.  Only the countdown needs it.
 */

static void eye_ui_tick(void)
{
  static int64_t last_info;

  int64_t now;
  int     left;

  g_ui_frames++;

  /* The live page is refreshed about once a second.  Redrawing it every
   * frame would flood the console and gain nothing a human could read.
   */

  if (g_ui_page == EYE_UI_PAGE_INFO)
    {
      now = eye_ui_ms();

      if (now - last_info >= 1000)
        {
          last_info = now;
          lv_label_set_text(g_ui_text_page, eye_ui_page_text());
        }

      return;
    }

  if (g_ui_page != EYE_UI_PAGE_PHOTO ||
      g_photo_state != EYE_UI_PHOTO_WAIT)
    {
      return;
    }

  /* Second chance to take the shot.  The loop's own call sits on the verdict
   * path, which a face has to be found to reach - and losing the face during
   * the countdown would otherwise leave it stuck at "1" with nothing saved.
   * Double saving is impossible because the state changes on the first one.
   */

  eye_ui_photo_maybe_save();

  now  = eye_ui_ms();
  left = (int)((g_photo_until - now + 999) / 1000);

  if (left > 0)
    {
      g_photo_shown = left;
    }

  /* The shot itself is taken earlier in the loop, before the overlay is
   * drawn; all that is left here is to show the digit.
   */
}

/****************************************************************************
 * Name: eye_ui_feed
 *
 * Description:
 *   Debounce one classifier verdict and act on it once it has held long
 *   enough.  A verdict has to hold *continuously* - a single stray "other"
 *   restarts the timer - and nothing is accepted for EYE_UI_COOLDOWN after a
 *   trigger, which is what stops one long glance from stepping through the
 *   whole grid.  "open" is deliberately inert: it is the resting state, and
 *   acting on it would fire the moment the user simply looked at the panel.
 *
 ****************************************************************************/

/****************************************************************************
 * Name: eye_ui_tone_hz
 *
 * Description:
 *   The pitch a verdict speaks in.  Three distinct notes, far enough apart
 *   to tell by ear across the room.
 *
 ****************************************************************************/

static uint32_t eye_ui_tone_hz(int cls)
{
  switch (cls)
    {
      case EYE_CAM_CLS_CLOSED:
        return 784;                     /* G5 - the confirm action */
      case EYE_CAM_CLS_LEFT:
        return 587;                     /* D5 - looking left */
      case EYE_CAM_CLS_RIGHT:
        return 988;                     /* B5 - looking right */
      default:
        return 660;                     /* E5 - anything else */
    }
}


static void eye_ui_feed(int cls, int conf)
{
  int64_t now = eye_ui_ms();
  int64_t hold;
  int     old;
  int     sel;

  g_ui_last_cls  = cls;
  g_ui_last_conf = conf;

  /* Frame rate for the information page, from the spacing of the verdicts. */

  if (g_ui_prev_ms != 0)
    {
      int64_t dt = now - g_ui_prev_ms;

      if (dt > 0 && dt < 5000)
        {
          g_ui_fps10 = (int)(10000 / dt);
        }
    }

  g_ui_prev_ms = now;

  if (now < g_ui_cooldown_until)
    {
      g_ui_tracking = false;
      stm32n6_buzzer_tone_stop();
      return;
    }

  if (!g_ui_tracking || cls != g_ui_last)
    {
      g_ui_last     = cls;
      g_ui_since    = now;
      g_ui_tracking = true;
      stm32n6_buzzer_tone_stop();
      return;
    }

  if (cls == EYE_CAM_CLS_OPEN || cls == EYE_CAM_CLS_OTHER)
    {
      g_ui_armed = true;
      stm32n6_buzzer_tone_stop();
      return;
    }

  /* Closing your eyes to enter a page leaves them closed, and the first thing
   * the page does must not be that very same blink.  An action therefore
   * requires a resting verdict first, so a blink is always a fresh decision
   * rather than something that can carry over.
   */

  if (cls == EYE_CAM_CLS_CLOSED && !g_ui_armed)
    {
      return;
    }

  /* An actionable verdict is being held.  On a second-level page the hold is
   * the action, so it sounds for as long as it lasts - which is also how the
   * user hears the hold being registered.  In the menu an action announces
   * itself once it fires, further down.
   */

  if (g_ui_page != EYE_UI_PAGE_GRID)
    {
      stm32n6_buzzer_tone_async(eye_ui_tone_hz(cls));
    }


  /* How long the verdict has to hold depends on what it will do, and that
   * depends on the page.  On a page, left means "back" rather than "previous
   * cell" - and it is given a longer hold, so a glance back at the cell that
   * was just stepped past does not throw the user out of the page.
   */

  if (g_ui_page != EYE_UI_PAGE_GRID && cls == EYE_CAM_CLS_LEFT)
    {
      hold = EYE_UI_HOLD_BACK;
    }
  else
    {
      hold = (cls == EYE_CAM_CLS_CLOSED) ? EYE_UI_HOLD_OK : EYE_UI_HOLD_NAV;
    }

  if ((now - g_ui_since) < hold)
    {
      return;
    }

  g_ui_tracking       = false;
  g_ui_cooldown_until = now + EYE_UI_COOLDOWN;
  /* The action is about to happen: end whatever note was being held and
   * mark the moment with one short one, so a step through the menu and a
   * completed action both have a voice.
   */

  stm32n6_buzzer_tone_stop();
  stm32n6_buzzer_tone(eye_ui_tone_hz(cls), 90);

  if (cls == EYE_CAM_CLS_CLOSED)
    {
      g_ui_armed = false;
    }

  /* ---- second-level pages --------------------------------------------- */

  if (g_ui_page != EYE_UI_PAGE_GRID)
    {
      if (cls == EYE_CAM_CLS_LEFT)
        {
          eye_cam_out("eye_ui: BACK to the menu (held %lldms)\n",
                      (long long)(now - g_ui_since));
          eye_ui_page_enter(EYE_UI_PAGE_GRID);
          return;
        }

      if (cls == EYE_CAM_CLS_RIGHT)
        {
          if (g_ui_cells == 0)
            {
              /* The shot page is the one camera-backed page with something
               * for the right verdict to do: ask for another shot.
               */

              if (g_ui_page == EYE_UI_PAGE_PHOTO)
                {
                  eye_ui_photo_retake();
                }

              return;      /* the preview has nothing to step through */
            }

          old = g_ui_sel;
          sel = (old + 1) % g_ui_cells;

          eye_cam_out("eye_ui: NEXT %d -> %d (%d%%, held %lldms)\n",
                      old, sel, conf, (long long)(now - g_ui_since));
          eye_ui_highlight(sel);
          return;
        }

      /* Closed eyes: act on the current cell, or leave the page if the cell
       * is the one that says "back".
       */

      old = g_ui_sel;

      switch (g_ui_page)
        {
          case EYE_UI_PAGE_LED:
            if (old >= EYE_UI_LED_CELLS - 1)
              {
                eye_cam_out("eye_ui: BACK to the menu\n");
                eye_ui_page_enter(EYE_UI_PAGE_GRID);
                return;
              }

            if (eye_ui_led_toggle(old))
              {
                eye_cam_out("eye_ui: LED %d -> %s (%d%%, held %lldms)\n",
                            old, (g_led_state & (1u << old)) ? "on" : "off",
                            conf, (long long)(now - g_ui_since));

                eye_ui_page_draw();
              }
            return;

          case EYE_UI_PAGE_INFO:
            if (old >= EYE_UI_INFO_CELLS - 1)
              {
                eye_cam_out("eye_ui: BACK to the menu\n");
                eye_ui_page_enter(EYE_UI_PAGE_GRID);
                return;
              }

            /* Refresh in place.  Re-entering the page would also reset the
             * selection to the first cell, which is not what was asked for
             * by closing eyes on cell three.
             */

            eye_cam_out("eye_ui: refresh info (cell %d)\n", old);
            eye_ui_page_draw();
            return;

          case EYE_UI_PAGE_ABOUT:
            if (old >= EYE_UI_ABOUT_CELLS - 1)
              {
                eye_cam_out("eye_ui: BACK to the menu\n");
                eye_ui_page_enter(EYE_UI_PAGE_GRID);
              }
            return;

          case EYE_UI_PAGE_CMD:
            if (old >= EYE_UI_CMD_CELLS - 1)
              {
                eye_cam_out("eye_ui: BACK to the menu\n");
                eye_ui_page_enter(EYE_UI_PAGE_GRID);
                return;
              }

            eye_cam_out("eye_ui: explain command %d\n", old);
            eye_ui_cmd_explain(old);
            return;

          case EYE_UI_PAGE_PHOTO:
            eye_ui_photo_retake();
            return;

          default:
            return;      /* the preview answers to nothing but left */
        }
    }

  /* ---- the menu grid --------------------------------------------------- */

  old = g_ui_sel;

  if (cls == EYE_CAM_CLS_CLOSED)
    {
      eye_cam_out("eye_ui: CONFIRM %d %s (%d%%, held %lldms)\n",
                  old, g_ui_text[old], conf, (long long)(now - g_ui_since));

      switch (old)
        {
          case 0:
            eye_cam_out("eye_ui: preview on the panel, look left to leave\n");
            eye_ui_page_enter(EYE_UI_PAGE_PREVIEW);
            break;

          case 1:
            eye_ui_page_enter(EYE_UI_PAGE_LED);
            break;

          case 2:
            eye_ui_page_enter(EYE_UI_PAGE_PHOTO);
            break;

          case 3:
            eye_ui_page_enter(EYE_UI_PAGE_INFO);
            break;

          case 4:
            eye_ui_page_enter(EYE_UI_PAGE_CMD);
            break;

          default:
            eye_ui_page_enter(EYE_UI_PAGE_ABOUT);
            break;
        }

      return;
    }

  sel = old + (cls == EYE_CAM_CLS_RIGHT ? 1 : -1);

  if (sel < 0)
    {
      sel = EYE_UI_COUNT - 1;
    }
  else if (sel >= EYE_UI_COUNT)
    {
      sel = 0;
    }

  eye_cam_out("eye_ui: MOVE %s %d -> %d %s (%d%%, held %lldms)\n",
              cls == EYE_CAM_CLS_RIGHT ? "right" : "left", old, sel,
              g_ui_text[sel], conf, (long long)(now - g_ui_since));

  eye_ui_highlight(sel);
}

static int eye_ui_run(void)
{
  lv_nuttx_dsc_t info;
  lv_nuttx_result_t result;

  lv_init();
  lv_nuttx_dsc_init(&info);

#ifdef CONFIG_LV_USE_NUTTX_LCD
  info.fb_path = "/dev/lcd0";
#endif

  lv_nuttx_init(&info, &result);

  if (result.disp == NULL)
    {
      eye_cam_out("eye_ui: LVGL display init failed\n");
      return 1;
    }

  eye_ui_build();

  eye_cam_out("eye_ui: panel up, %dx%d cells, select=%d\n",
              EYE_UI_COLS, EYE_UI_ROWS, g_ui_sel);

  while (!g_ui_quit)
    {
      uint32_t idle = lv_timer_handler();

      /* lv_timer_handler() returns how long until the next timer is due.
       * Sleeping exactly that long keeps the panel responsive without
       * spinning a 600 MHz core on a screen that is not changing.
       */

      if (idle < EYE_UI_MIN_IDLE_MS)
        {
          idle = EYE_UI_MIN_IDLE_MS;
        }

      usleep(idle * 1000);
    }

  lv_nuttx_deinit(&result);
  lv_deinit();

  return 0;
}

#endif /* CONFIG_GRAPHICS_LVGL */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: eye_cam_audio_cmd
 *
 * Description:
 *   "eye_cam audio <file.wav>": play a file through the speaker.
 *
 *   Kept beyond the bring-up because it is also the only check on the audio
 *   clock that does not need a scope: the driver reports how long the file
 *   took against how long its header says it should, and a wrong divider in
 *   the PLL2 chain shows up there as a proportional error instead of as
 *   "sounds a bit off".
 *
 ****************************************************************************/

static int eye_cam_tone_cmd(int argc, FAR char *argv[])
{
  uint32_t hz = 2000;
  uint32_t ms = 300;

  if (argc >= 3)
    {
      hz = (uint32_t)strtoul(argv[2], NULL, 0);
    }

  if (argc >= 4)
    {
      ms = (uint32_t)strtoul(argv[3], NULL, 0);
    }

  stm32n6_buzzer_initialize();
  eye_cam_out("tone: %lu Hz for %lu ms\n", (unsigned long)hz,
              (unsigned long)ms);
  stm32n6_buzzer_tone(hz, ms);
  return 0;
}


static int eye_cam_beep_cmd(int argc, FAR char *argv[])
{
  uint32_t beep_ms = 200;

  if (argc >= 3)
    {
      beep_ms = (uint32_t)strtoul(argv[2], NULL, 0);
    }

  stm32n6_buzzer_initialize();
  eye_cam_out("buzzer: %lu ms\n", (unsigned long)beep_ms);
  stm32n6_buzzer_beep(beep_ms);
  return 0;
}


#if defined(CONFIG_STM32_SAI1)

extern int stm32n6_audio_play_wav(FAR const char *path);

static int eye_cam_audio_cmd(int argc, FAR char *argv[])
{
  if (argc < 3)
    {
      eye_cam_out("eye_cam: usage: eye_cam audio <file.wav>\n");
      return 1;
    }

  return stm32n6_audio_play_wav(argv[2]) < 0 ? 1 : 0;
}

#endif /* CONFIG_STM32_SAI1 */

int main(int argc, FAR char *argv[])
{
  struct stm32n6_aton_invoke_params params;
  struct stm32n6_aton_status_s status;
  struct v4l2_format fmt;
  struct fb_planeinfo_s pinfo;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  FAR int8_t *input;
  FAR int8_t *output;
  int iters = 0;
  int hold_ms = EYE_CAM_HOLD_MS;
  int vfd = -1;
  int fbfd = -1;
  uintptr_t band_start;
  uintptr_t band_end;
  uintptr_t cstart;
  uintptr_t cend;
  int fret;
  int i;

  /* Tone check: two frequencies four octaves apart tell an active buzzer
   * from a passive one by ear.
   */

  if (argc >= 2 && strcmp(argv[1], "tone") == 0)
    {
      return eye_cam_tone_cmd(argc, argv);
    }


  /* Buzzer check: no panel and no audio path needed. */

  if (argc >= 2 && strcmp(argv[1], "beep") == 0)
    {
      return eye_cam_beep_cmd(argc, argv);
    }


#if defined(CONFIG_STM32_SAI1)
  /* Speaker bring-up, independent of the panel. */

  if (argc >= 2 && strcmp(argv[1], "audio") == 0)
    {
      return eye_cam_audio_cmd(argc, argv);
    }
#endif

#if defined(CONFIG_GRAPHICS_LVGL)
  /* Diagnostic entry points: the HyperRAM bring-up lab */

  if (argc >= 2 && strcmp(argv[1], "xspi") == 0)
    {
      return eye_cam_xspi_cmd(argc, argv);
    }

  if (argc == 2 && strcmp(argv[1], "xspitest") == 0)
    {
      return eye_cam_xspi_probe();
    }

  /* "eye_cam panel": the panel on its own, no camera.  Kept because it is
   * the quickest way to tell a UI problem from a capture problem.
   */

  if (argc == 2 && strcmp(argv[1], "panel") == 0)
    {
      return eye_ui_run();
    }

  /* No arguments: the eye-controlled panel.  This used to return early into
   * eye_ui_run() - the capture stream and the panel shared one framebuffer,
   * so the grid could not survive a frame.  The capture buffer now lives in
   * the HyperRAM, so this runs the full capture/inference chain instead and
   * drives the grid from the verdict.
   */

  if (argc == 1)
    {
      g_ui_mode = true;
    }
#endif

  for (i = 0; i < 32; i++)
    {
      g_exp5[i] = (uint8_t)((i * 255) / 31);
    }

  for (i = 0; i < 64; i++)
    {
      g_exp6[i] = (uint8_t)((i * 255) / 63);
    }

  /* The taps are built lazily, from whatever rectangle the first frame
   * wants - the fixed one until the detector has placed a crop.
   */

  if (argc > 1)
    {
      iters = atoi(argv[1]);
    }

  if (argc > 2)
    {
      hold_ms = atoi(argv[2]);
    }

  /* eye_cam capture <label> [count]
   *
   * One class per run, and the run stops when the batch is full.
   * The next label is a separate invocation so a batch can be checked
   * - or redone - before the next one starts.
   */

  if (argc > 1 && strcmp(argv[1], "capture") == 0)
    {
      g_cap_label = (argc > 2) ? argv[2] : "sample";
      g_cap_count = (argc > 3) ? atoi(argv[3]) : EYE_CAM_CAP_DEF_N;

      /* Optional "fixed [w h]": for the class that is defined by
       * having no face in it, where a detector-driven crop can never
       * fire.
       */

      if (argc > 4 && strcmp(argv[4], "fixed") == 0)
        {
          g_cap_fixed = 1;

          if (argc > 6)
            {
              g_cap_fixed_w = atoi(argv[5]);
              g_cap_fixed_h = atoi(argv[6]);
            }
        }
    }

  g_aiefd = open("/dev/aie0", O_RDWR);
  if (g_aiefd < 0)
    {
      fprintf(stderr, "eye_cam: open /dev/aie0 failed: %d\n", errno);
      return 1;
    }

  if (ioctl(g_aiefd, AIE_CMD_LOAD, 0) < 0)
    {
      eye_cam_out("eye_cam: AIE_CMD_LOAD failed: %d\n", errno);
      close(g_aiefd);
      return 1;
    }

  memset(&status, 0, sizeof(status));
  status.model_id = STM32N6_ATON_MODEL_PRIMARY;
  if (ioctl(g_aiefd, STM32N6_ATON_CMD_GET_STATUS, (unsigned long)&status) < 0)
    {
      eye_cam_out("eye_cam: GET_STATUS failed: %d\n", errno);
      close(g_aiefd);
      return 1;
    }

  if (status.input_size != EYE_CAM_IN_LEN || status.output_size != EYE_CAM_OUT_LEN)
    {
      eye_cam_out("eye_cam: refusing to run: tensor sizes are %lu/%lu, this "
                  "app is built for the blink model (%d/%d)\n",
                  (unsigned long)status.input_size,
                  (unsigned long)status.output_size,
                  EYE_CAM_IN_LEN, EYE_CAM_OUT_LEN);
      close(g_aiefd);
      return 1;
    }

  /* The engines read the input from the address the epoch program was
   * published with - writing the frame anywhere else leaves the result
   * unchanged frame after frame, so take it from the driver.
   */

  input = (FAR int8_t *)(uintptr_t)(status.input_addr != 0 ?
                                    status.input_addr : EYE_CAM_INPUT_ADDR);

  output = (FAR int8_t *)(uintptr_t)(status.output_addr != 0 ?
                                     status.output_addr : EYE_CAM_OUTPUT_ADDR);

  eye_cam_out("eye_cam: blink model, input %lu B at 0x%08lx (%s), output %lu B "
              "at 0x%08lx (%s)\n",
              (unsigned long)status.input_size, (unsigned long)(uintptr_t)input,
              status.input_addr != 0 ? "driver registered" : "fallback",
              (unsigned long)status.output_size,
              (unsigned long)(uintptr_t)output,
              status.output_addr != 0 ? "model owned" : "fallback");

  /* Second slot: the face detector the two stage pipeline localises the eye
   * with.  A refusal here is not fatal - the classifier alone still runs -
   * so report it and carry on.
   */

  fret = eye_cam_face_preflight(g_aiefd, input);
  if (fret < 0)
    {
      eye_cam_out("eye_cam: face slot not usable (%d): classifier only\n",
                  fret);
    }
  else
    {
      g_face_ok = 1;
    }

  vfd = open("/dev/video0", O_RDONLY);
  if (vfd < 0)
    {
      eye_cam_out("eye_cam: open /dev/video0 failed: %d\n", errno);
      close(g_aiefd);
      return 1;
    }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = EYE_CAM_FRAME_W;
  fmt.fmt.pix.height      = EYE_CAM_FRAME_H;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;

  if (ioctl(vfd, VIDIOC_S_FMT, &fmt) < 0)
    {
      eye_cam_out("eye_cam: VIDIOC_S_FMT failed: %d\n", errno);
      close(vfd);
      close(g_aiefd);
      return 1;
    }

  if (ioctl(vfd, VIDIOC_STREAMON, &type) < 0)
    {
      eye_cam_out("eye_cam: VIDIOC_STREAMON failed: %d\n", errno);
      close(vfd);
      close(g_aiefd);
      return 1;
    }

  fbfd = open("/dev/fb0", O_RDONLY);
  if (fbfd < 0)
    {
      eye_cam_out("eye_cam: open /dev/fb0 failed: %d\n", errno);
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      close(vfd);
      close(g_aiefd);
      return 1;
    }

  if (ioctl(fbfd, FBIOGET_PLANEINFO, &pinfo) < 0)
    {
      eye_cam_out("eye_cam: FBIOGET_PLANEINFO failed: %d\n", errno);
      close(fbfd);
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      close(vfd);
      close(g_aiefd);
      return 1;
    }

  g_fb        = (FAR uint16_t *)(uintptr_t)pinfo.fbmem;
  g_stride_px = pinfo.stride / 2;

#if defined(CONFIG_GRAPHICS_LVGL)
  /* Remember where the panel's buffer is before g_fb is re-pointed at the
   * capture buffer: the preview page copies frames back to here.
   */

  g_lcd_fb        = g_fb;
  g_lcd_stride_px = g_stride_px;

  if (g_ui_mode)
    {
      /* Hand the panel's framebuffer to LVGL and send the capture stream to
       * the HyperRAM.  Re-pointing g_fb is all the rest of this file needs:
       * the detector, the crop and the overlay all work through it, so the
       * overlay simply ends up on the off-screen copy.
       */

      stm32n6_video_set_capture_dest(EYE_UI_CAM_ADDR);

      g_fb        = (FAR uint16_t *)EYE_UI_CAM_ADDR;
      g_stride_px = EYE_CAM_FRAME_W;

      if (eye_ui_lvgl_start() != 0)
        {
          eye_cam_out("eye_cam: LVGL init failed, running without a panel\n");
          g_ui_mode = false;
        }
      else
        {
          eye_cam_out("eye_ui: capture -> 0x%08lx, panel on /dev/lcd0\n",
                      (unsigned long)EYE_UI_CAM_ADDR);
        }
    }
#endif

  /* The camera writes the same buffer the LTDC scans out.  Keeping the frame
   * in dcache is only useful if the two ends agree on which copy is current:
   * the DCMIPP fills memory, so the CPU has to drop its stale lines before
   * reading the ROI, and push its own drawing back out afterwards.
   *
   * The band spans everything this app draws: from above the verdict label
   * down past the bottom edge of the box.  Deriving both ends from the drawing
   * geometry is what keeps it honest.
   */

  band_start = (uintptr_t)(g_fb + (size_t)(EYE_CAM_LABEL_Y - EYE_CAM_BAND_PAD) *
                           g_stride_px);
  band_end   = (uintptr_t)(g_fb + (size_t)(EYE_CAM_ROI_Y + EYE_CAM_ROI_H + 60) *
                           g_stride_px);

  eye_cam_out("eye_cam: cache band rows %d..%d (label at %d, ROI at %d)\n",
              EYE_CAM_LABEL_Y - EYE_CAM_BAND_PAD,
              EYE_CAM_ROI_Y + EYE_CAM_ROI_H + 60, EYE_CAM_LABEL_Y, EYE_CAM_ROI_Y);

  eye_cam_out("eye_cam: ROI x=%d y=%d %dx%d -> %dx%d int8 planar, panel %p "
              "stride %u\n",
              EYE_CAM_ROI_X, EYE_CAM_ROI_Y, EYE_CAM_ROI_W, EYE_CAM_ROI_H,
              EYE_CAM_NN_W, EYE_CAM_NN_H, (FAR void *)g_fb,
              (unsigned)pinfo.stride);

  /* Open the capture directory before the first frame, so a missing
   * card is reported once at the start rather than as a file error on
   * every frame.
   */

  if (g_cap_count > 0)
    {
      if (mkdir(EYE_CAM_CAP_DIR, 0777) < 0 && errno != EEXIST)
        {
          eye_cam_out("eye_cam: capture off: mkdir %s failed: %d\n",
                      EYE_CAM_CAP_DIR, errno);
          g_cap_count = 0;
        }
      else if ((g_cap_seq = eye_cam_cap_next_seq(g_cap_label)) < 0)
        {
          eye_cam_out("eye_cam: capture off: no free file number\n");
          g_cap_count = 0;
        }
      else
        {
          g_cap_meta = fopen(EYE_CAM_CAP_META, "a");
          if (g_cap_meta != NULL && ftell(g_cap_meta) == 0)
            {
              fprintf(g_cap_meta,
                      "# label seq crop_x crop_y crop_w crop_h "
                      "score eye_x eye_y dx\n");
              fflush(g_cap_meta);
            }

          eye_cam_out("eye_cam: capture \"%s\" %d frames to %s, "
                      "from %04d%s\n",
                      g_cap_label, g_cap_count, EYE_CAM_CAP_DIR,
                      g_cap_seq,
                      g_cap_fixed ? " (fixed frame)" : "");
        }
    }

  for (i = 0; i < iters || iters <= 0; i++)
    {
      clock_t tframe = clock_systime_ticks();
      clock_t t0;
      int   ret;

      /* A fresh frame, then freeze it.  The overlay is drawn after the
       * inference and the camera writes the very buffer it lives in, so a
       * frame arriving in between would erase the result before it is seen.
       */

      if (read(vfd, NULL, 0) < 0)
        {
          eye_cam_out("eye_cam: frame sync failed: %d\n", errno);
          break;
        }

      eye_cam_stage_add(EYE_CAM_ST_READ, clock_systime_ticks() - tframe);

      t0 = clock_systime_ticks();
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      eye_cam_stage_add(EYE_CAM_ST_STREAMOFF, clock_systime_ticks() - t0);

      /* Stage one: locate the face on the frame just frozen.  Both slots
       * were registered at the same address, so the classifier's input
       * buffer doubles as the detector's - which is only safe as long as
       * the two never overlap in time.  The whole of stage one, from the
       * fill to the last read of the activations, happens before the crop
       * below writes that buffer again.
       */

      /* Declared here rather than inside the detector's block because the
       * overlay, further down, draws the result.
       */

      struct eye_cam_face_s face;
      int face_ok = 0;

      memset(&face, 0, sizeof(face));

      if (g_face_ok)
        {
          t0 = clock_systime_ticks();

          /* The detector looks at all of the frame, not a band of it. */

          up_invalidate_dcache((uintptr_t)g_fb,
                               (uintptr_t)g_fb +
                               (size_t)g_stride_px * EYE_CAM_FRAME_H);

          eye_cam_face_prep(g_fb, pinfo.stride, input);
          up_clean_dcache((uintptr_t)input,
                          (uintptr_t)input + EYE_CAM_FACE_IN_LEN);

          memset(&params, 0, sizeof(params));
          params.model_id    = STM32N6_ATON_MODEL_FACE;
          params.input       = input;
          params.output      = g_face_out;
          params.input_size  = EYE_CAM_FACE_IN_LEN;
          params.output_size = g_face_out_len;

          ret = ioctl(g_aiefd, AIE_CMD_FEED_INPUT,
                      (unsigned long)&params);
          if (ret < 0)
            {
              eye_cam_out("eye_cam: face inference failed: %d (errno %d)\n",
                          ret, errno);
              ioctl(vfd, VIDIOC_STREAMON, &type);
              break;
            }

          up_invalidate_dcache((uintptr_t)g_face_out,
                               (uintptr_t)g_face_out + g_face_out_len);

          /* Three short lines: the polled writer cuts anything longer.
           * The PC side prints the same numbers off the same frame, so
           * the two can be compared field by field.
           */

          face_ok = eye_cam_face_decode(g_face_out, &face);
          if (face_ok)
            {
              /* 2.0 pupil distances wide, bottom edge a quarter of the
               * way up: the geometry the PC side settled on, where 115
               * of 115 full frames cropped onto the eyes, with the
               * below-the-eyes strip trimmed off.
               */

              g_crop_w = (int)(EYE_CAM_CROP_K * face.dx + 0.5f);
              g_crop_h = (int)((float)g_crop_w *
                               (float)EYE_CAM_ROI_H /
                               (float)EYE_CAM_ROI_W *
                               EYE_CAM_CROP_H_SCALE + 0.5f);
              g_crop_x = (int)(face.ex - g_crop_w * 0.5f + 0.5f);
              g_crop_y = (int)(face.ey -
                               EYE_CAM_CROP_EYE_REL * (float)g_crop_h +
                               0.5f);
              g_crop_ok = 1;

              eye_cam_out("eye_cam: face %.3f st=%lu i=%lu\n",
                          (double)face.score,
                          (unsigned long)face.stride,
                          (unsigned long)face.anchor);
              g_ui_face_ok    = true;
              g_ui_face_score = face.score;

              eye_cam_out("eye_cam: eye=(%.1f,%.1f) dx=%.1f\n",
                          (double)face.ex, (double)face.ey,
                          (double)face.dx);
              eye_cam_out("eye_cam: kp R=(%.1f,%.1f) L=(%.1f,%.1f)\n",
                          (double)face.kp[0], (double)face.kp[1],
                          (double)face.kp[2], (double)face.kp[3]);
            }
          else
            {
              g_ui_face_ok    = false;
              g_ui_face_score = face.score;

              eye_cam_out("eye_cam: no face (best %.3f)\n",
                          (double)face.score);
            }

          eye_cam_stage_add(EYE_CAM_ST_FACE, clock_systime_ticks() - t0);
        }

      /* Fixed-frame mode pins the crop here, after the detector has had
       * its turn, so this is what the rest of the frame uses.  The
       * detector still runs - it costs little next to the inference and
       * its output is what the console reports - but nothing depends on
       * it having found anything.
       */

      if (g_cap_fixed)
        {
          g_crop_w = g_cap_fixed_w;
          g_crop_h = g_cap_fixed_h;
          g_crop_x = (EYE_CAM_FRAME_W - g_crop_w) / 2;
          g_crop_y = (EYE_CAM_FRAME_H - g_crop_h) / 2;
          g_crop_ok = 1;
        }

      t0 = clock_systime_ticks();

      /* Whatever rectangle the detector last placed, held inside the
       * frame so the band below is a valid range to invalidate.
       */

      {
        int y0 = g_crop_y - 4;
        int y1 = g_crop_y + g_crop_h + 4;

        if (y0 < 0)
          {
            y0 = 0;
          }

        if (y1 > EYE_CAM_FRAME_H)
          {
            y1 = EYE_CAM_FRAME_H;
          }

        up_invalidate_dcache((uintptr_t)(g_fb +
                                         (size_t)y0 * g_stride_px),
                             (uintptr_t)(g_fb +
                                         (size_t)y1 * g_stride_px));
      }

      eye_cam_taps_for(g_crop_w, g_crop_h);
      eye_cam_roi_to_input(g_fb, pinfo.stride, input, g_crop_x, g_crop_y);
      up_clean_dcache((uintptr_t)input, (uintptr_t)input + EYE_CAM_IN_LEN);

      /* Capture the crop that was just built, here rather than after
       * the overlay: the box is drawn along the crop's own edge, and a
       * file written afterwards would carry the annotation.
       *
       * Only frames the detector actually placed are written - a crop
       * left over from the previous frame does not describe this one,
       * and a training image whose box is wrong is worse than a
       * missing one.
       *
       * Fixed-frame mode is the exception: there the crop describes
       * the frame by construction, and the detector finding nothing is
       * the normal case rather than a failure.
       */

      if (g_cap_count > 0 && g_cap_saved < g_cap_count &&
          (g_cap_fixed || (face_ok && g_crop_ok)))
        {
          if (eye_cam_cap_save(g_cap_label, g_cap_seq + g_cap_saved,
                               g_stride_px, &face, g_cap_meta) == 0)
            {
              g_cap_saved++;

              if ((g_cap_saved % 10) == 0 || g_cap_saved == g_cap_count)
                {
                  eye_cam_out("eye_cam: capture %s %d/%d (%dx%d)\n",
                              g_cap_label, g_cap_saved, g_cap_count,
                              g_crop_w, g_crop_h);
                }
            }
        }

      if ((i % 10) == 0)
        {
          eye_cam_input_probe(input);
        }

      eye_cam_stage_add(EYE_CAM_ST_CROP, clock_systime_ticks() - t0);

      memset(&params, 0, sizeof(params));
      params.input       = input;
      params.output      = output;
      params.input_size  = EYE_CAM_IN_LEN;
      params.output_size = EYE_CAM_OUT_LEN;

      t0 = clock_systime_ticks();
      ret = ioctl(g_aiefd, AIE_CMD_FEED_INPUT, (unsigned long)&params);
      eye_cam_stage_add(EYE_CAM_ST_INFER, clock_systime_ticks() - t0);

      if (ret < 0)
        {
          eye_cam_out("eye_cam: inference failed: %d (errno %d)\n", ret, errno);
          ioctl(vfd, VIDIOC_STREAMON, &type);
          break;
        }

      t0 = clock_systime_ticks();
      up_invalidate_dcache((uintptr_t)output,
                           (uintptr_t)output + EYE_CAM_OUT_LEN);

      /* CrossEntropyLoss was the training criterion, so these bytes are
       * logits: the largest one is the class.  The index order is the one
       * eye_common.task_classes() builds and the directory scan produced.
       */

      {
        float s[EYE_CAM_NCLS];
        float p[EYE_CAM_NCLS];
        float top;
        float sum = 0.0f;
        int   best = 0;
        int   k;

        for (k = 0; k < EYE_CAM_NCLS; k++)
          {
            s[k] = ((float)output[k] - (float)EYE_CAM_OUT_ZP) *
                   EYE_CAM_OUT_SCALE;

            if (s[k] > s[best])
              {
                best = k;
              }
          }

        /* Softmax, shifted by the maximum so the exponentials stay in range.
         * It is only there for the percentage on the panel - the decision
         * itself is the argmax above.
         */

        top = s[best];
        for (k = 0; k < EYE_CAM_NCLS; k++)
          {
            p[k] = expf(s[k] - top);
            sum += p[k];
          }

#if defined(CONFIG_GRAPHICS_LVGL)
        if (g_ui_mode)
          {
            /* Drive the grid from this verdict and give LVGL its slice.  The
             * overlay further down still runs, but it lands in the HyperRAM
             * frame rather than on the screen.
             */

            eye_ui_feed(best, (int)(100.0f * p[best] / sum + 0.5f));

            /* On a camera-backed page the panel is showing the camera, so
             * LVGL must not flush here: the frame goes down at the tail of
             * the loop and the badge is drawn over it.  Its timers keep
             * their state meanwhile.
             */

            if (!eye_ui_camera_mode())
              {
                lv_timer_handler();
              }
          }
#endif

        /* The shot goes here, ahead of the overlay, so the file holds the
         * camera's frame and not the annotated one.  The live view below is
         * unaffected: it keeps its box, which is worth having while framing.
         */

#if defined(CONFIG_GRAPHICS_LVGL)
        if (g_ui_mode)
          {
            eye_ui_photo_maybe_save();
          }
#endif

        /* Frame the window the classifier actually saw, in the colour of the
         * verdict, and print the verdict with it.
         *
         * Drawn one stroke wider than the cyan box rather than on top of
         * it, so the two stay distinguishable: the colour carries the
         * verdict, the cyan says which pixels were cropped.
         *
         * Still outside the cropped region, so the annotation can never
         * become part of the next iteration's input.
         */

        eye_cam_rect(g_crop_x - EYE_CAM_VERDICT_PAD,
                     g_crop_y - EYE_CAM_VERDICT_PAD,
                     g_crop_w + 2 * EYE_CAM_VERDICT_PAD,
                     g_crop_h + 2 * EYE_CAM_VERDICT_PAD,
                     3, g_eye_cam_color[best]);

        {
          char txt[40];
          int  ly = g_crop_y - EYE_CAM_LABEL_GAP;

          snprintf(txt, sizeof(txt), "%s %d%%", g_eye_cam_name[best],
                   (int)(100.0f * p[best] / sum + 0.5f));

          /* Above the frame when there is room, below it when the face
           * sits high enough that there is not - a label off the top of
           * the screen is worth nothing.
           */

          if (ly < EYE_CAM_LABEL_GAP)
            {
              ly = g_crop_y + g_crop_h + EYE_CAM_LABEL_GAP;
            }

          eye_cam_label(g_crop_x, ly, txt,
                        EYE_CAM_LABEL_SCALE, g_eye_cam_color[best]);
        }

        /* Where the detector would put the crop window, drawn beside the
         * classifier's fixed ROI in a colour no class uses.  The point is
         * to check the geometry by eye before the crop is switched over
         * to it - watching the box land on the eyes beats reading
         * coordinates off the console, which cannot show whether the box
         * is right relative to the frame.
         *
         * The band below was sized for the fixed ROI.  This box can reach
         * outside it, and a line left sitting in the cache would be
         * scanned out as the stale copy, so the push widens to cover
         * whatever was actually drawn.
         */

        cstart = band_start;
        cend   = band_end;

        if (g_crop_ok)
          {
            int cx = g_crop_x;
            int cy = g_crop_y;
            int cw = g_crop_w;
            int ch = g_crop_h;
            int y0;
            int y1;

            /* The rectangle the classifier was actually fed, which is
             * not the one this frame's keypoints describe when the
             * detector missed and the previous crop was kept.
             */

            eye_cam_rect(cx, cy, cw, ch, 2, EYE_CAM_COLOR_CROP);

            /* A square on each eye: a frame where the box is right but
             * the keypoints are not then shows up immediately.
             */

            if (face_ok)
              {
                eye_cam_rect((int)face.kp[0] - 2, (int)face.kp[1] - 2, 5,
                             5, 1, EYE_CAM_COLOR_CROP);
                eye_cam_rect((int)face.kp[2] - 2, (int)face.kp[3] - 2, 5,
                             5, 1, EYE_CAM_COLOR_CROP);
              }

            /* Everything drawn sits near the crop: the verdict frame
             * just outside it, the label a line beyond that, the eye
             * markers inside.  Deriving the push from the crop covers
             * all three wherever the face happens to be.
             */

            y0 = cy - EYE_CAM_DRAW_MARGIN;
            y1 = cy + ch + EYE_CAM_DRAW_MARGIN;

            if (y0 < 0)
              {
                y0 = 0;
              }

            if (y1 > EYE_CAM_FRAME_H)
              {
                y1 = EYE_CAM_FRAME_H;
              }

            if (y0 < y1)
              {
                uintptr_t fs = (uintptr_t)(g_fb +
                                           (size_t)y0 * g_stride_px);
                uintptr_t fe = (uintptr_t)(g_fb +
                                           (size_t)y1 * g_stride_px);

                if (fs < cstart)
                  {
                    cstart = fs;
                  }

                if (fe > cend)
                  {
                    cend = fe;
                  }
              }
          }

        up_clean_dcache(cstart, cend);

        eye_cam_stage_add(EYE_CAM_ST_OVERLAY, clock_systime_ticks() - t0);

        if (i == 0 || (i % 5) == 0)
          {
            eye_cam_out("eye_cam: %d %s (%d%%) logits %d/%d/%d/%d/%d %luus\n",
                        i, g_eye_cam_name[best],
                        (int)(100.0f * p[best] / sum + 0.5f),
                        (int)(s[0] * 100.0f), (int)(s[1] * 100.0f),
                        (int)(s[2] * 100.0f), (int)(s[3] * 100.0f),
                        (int)(s[4] * 100.0f),
                        (unsigned long)params.usec);
          }
      }

#if defined(CONFIG_GRAPHICS_LVGL)
      if (g_ui_mode)
        {
          eye_ui_tick();

          /* Frame first, badge second.  The blit covers the whole panel, so
           * the badge has to be redrawn after it every time.
           */

          if (eye_ui_camera_mode())
            {
              eye_ui_preview_blit();
              eye_ui_badge_draw();
              lv_timer_handler();
            }
        }
#endif

      t0 = clock_systime_ticks();
      usleep((useconds_t)hold_ms * 1000);
      eye_cam_stage_add(EYE_CAM_ST_HOLD, clock_systime_ticks() - t0);

      t0 = clock_systime_ticks();
      ioctl(vfd, VIDIOC_STREAMON, &type);
      eye_cam_stage_add(EYE_CAM_ST_STREAMON, clock_systime_ticks() - t0);

      eye_cam_stage_add(EYE_CAM_ST_TOTAL, clock_systime_ticks() - tframe);

      if ((i % 15) == 0)
        {
          eye_cam_stage_report(i + 1);
        }

      if (g_cap_count > 0 && g_cap_saved >= g_cap_count)
        {
          break;
        }
    }

  eye_cam_stage_report(i);

  if (g_cap_meta != NULL)
    {
      fclose(g_cap_meta);
      g_cap_meta = NULL;
    }

  if (g_cap_count > 0)
    {
      eye_cam_out("eye_cam: capture %s: %d/%d frames, files %04d..%04d\n",
                  g_cap_label, g_cap_saved, g_cap_count, g_cap_seq,
                  g_cap_seq + (g_cap_saved > 0 ? g_cap_saved - 1 : 0));
    }

  eye_cam_out("eye_cam: done\n");

#if defined(CONFIG_GRAPHICS_LVGL)
  if (g_ui_mode)
    {
      eye_ui_lvgl_stop();
    }
#endif

  ioctl(vfd, VIDIOC_STREAMOFF, &type);
  close(fbfd);
  close(vfd);
  close(g_aiefd);
  return 0;
}
