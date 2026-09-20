/****************************************************************************
 * apps/examples/eye_guide/eye_guide_main.c
 *
 * ROI framing aid for eye-movement dataset capture.
 *
 * Why this exists
 * ---------------
 * The eye ROI is fixed at (x=235, y=155, 300x145) inside the 800x480 camera
 * frame, and training + inference must see exactly the same framing.  In the
 * first capture round there was no visual reference, and the two classes
 * drifted apart: the closed-eye batch ended up roughly 1.4x further from the
 * camera than the open-eye batch (the whole face fitted in the frame instead
 * of just the eyes).  That is a shortcut a classifier learns happily --
 * "big face = open, small face = closed" -- and it does not survive
 * deployment.
 *
 * What it does
 * ------------
 * Streams the camera and continuously redraws the ROI rectangle, four corner
 * ticks, a centre cross and the eye-height reference line straight into the
 * LTDC framebuffer.  The camera writes that same buffer, so every drawn frame
 * is overwritten by the next one: the guide visibly blinks a few times per
 * second, which is plenty for aligning.  For a rock-steady box, lower the
 * frame rate first:
 *
 *   nsh> imx335_tune fps 5
 *   nsh> eye_guide
 *
 * Geometry (keep in sync with the dataset and the inference path)
 * ---------------------------------------------------------------
 *   ROI          x=235 y=155 300x145   (the calibration-frozen window)
 *   eye row      +28 px inside the ROI  (measured: open 28.6, close 28.4)
 *
 * Usage
 * -----
 *   nsh> eye_guide          # run until Ctrl-C
 *   nsh> eye_guide 60       # run for 60 seconds
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/videoio.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include <nuttx/cache.h>
#include <nuttx/video/fb.h>

/* ---- Geometry: must match the calibration / training / inference window --- */

#define GUIDE_FRAME_W   800
#define GUIDE_FRAME_H   480

#define GUIDE_ROI_X     235
#define GUIDE_ROI_Y     155
#define GUIDE_ROI_W     300
#define GUIDE_ROI_H     145

/* Eye centre height inside the ROI (measured on both batches). */

#define GUIDE_EYE_ROW   28

/* ---- Colours, RGB565 ----------------------------------------------------- */

#define COL_BOX         0x07e0   /* green  : the ROI itself          */
#define COL_TICK        0xffe0   /* yellow : corner ticks            */
#define COL_REF         0x8410   /* grey   : cross / reference line  */

/* Redraw rate.  25 Hz against a 30 fps stream: the box stays visible a good
 * part of the time, and the flicker is a useful signal that it is alive.
 */

#define GUIDE_REDRAW_MS 40

static uint16_t *g_fb;
static uint32_t g_stride_px;

/****************************************************************************
 * Drawing primitives (same shape as palm_cam's: they write direct into the
 * buffer the LTDC scans out, because this app already holds that pointer).
 ****************************************************************************/

static void guide_hline(int x, int y, int len, uint16_t color)
{
  FAR uint16_t *row;

  if (g_fb == NULL || len <= 0 || y < 0 || y >= GUIDE_FRAME_H)
    {
      return;
    }

  if (x < 0)
    {
      len += x;
      x = 0;
    }

  if (x + len > GUIDE_FRAME_W)
    {
      len = GUIDE_FRAME_W - x;
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

static void guide_vline(int x, int y, int len, uint16_t color)
{
  int i;

  for (i = 0; i < len; i++)
    {
      guide_hline(x, y + i, 1, color);
    }
}

static void guide_rect(int x, int y, int w, int h, int t, uint16_t color)
{
  int i;

  if (w <= 0 || h <= 0)
    {
      return;
    }

  for (i = 0; i < t; i++)
    {
      guide_hline(x, y + i, w, color);
      guide_hline(x, y + h - 1 - i, w, color);
      guide_vline(x + i, y, h, color);
      guide_vline(x + w - 1 - i, y, h, color);
    }
}

/* Dashed horizontal line: 4 px on, 4 px off.  Used for the eye-height
 * reference so it does not hide the very feature it is marking.
 */

static void guide_dashes_h(int x, int y, int len, uint16_t color)
{
  int i;

  for (i = 0; i < len; i += 8)
    {
      guide_hline(x + i, y, (len - i >= 4) ? 4 : (len - i), color);
    }
}

static void guide_draw(void)
{
  int cx = GUIDE_ROI_X + GUIDE_ROI_W / 2;
  int cy = GUIDE_ROI_Y + GUIDE_ROI_H / 2;
  int er = GUIDE_ROI_Y + GUIDE_EYE_ROW;
  const int l = 26;

  /* The ROI itself: 2 px green. */

  guide_rect(GUIDE_ROI_X, GUIDE_ROI_Y, GUIDE_ROI_W, GUIDE_ROI_H, 2, COL_BOX);

  /* Corner ticks: 3 px yellow, so the box reads at a glance from arm's length
   * even while it flickers.
   */

  guide_hline(GUIDE_ROI_X, GUIDE_ROI_Y, l, COL_TICK);
  guide_vline(GUIDE_ROI_X, GUIDE_ROI_Y, l, COL_TICK);
  guide_hline(GUIDE_ROI_X + GUIDE_ROI_W - l, GUIDE_ROI_Y, l, COL_TICK);
  guide_vline(GUIDE_ROI_X + GUIDE_ROI_W - 3, GUIDE_ROI_Y, l, COL_TICK);
  guide_hline(GUIDE_ROI_X, GUIDE_ROI_Y + GUIDE_ROI_H - 3, l, COL_TICK);
  guide_vline(GUIDE_ROI_X, GUIDE_ROI_Y + GUIDE_ROI_H - l, l, COL_TICK);
  guide_hline(GUIDE_ROI_X + GUIDE_ROI_W - l, GUIDE_ROI_Y + GUIDE_ROI_H - 3, l,
              COL_TICK);
  guide_vline(GUIDE_ROI_X + GUIDE_ROI_W - 3, GUIDE_ROI_Y + GUIDE_ROI_H - l, l,
              COL_TICK);

  /* Centre cross: horizontal centring aid. */

  guide_hline(cx - 22, cy, 44, COL_REF);
  guide_vline(cx, cy - 14, 28, COL_REF);

  /* Eye-height reference: put both eyes on this dashed line.  This is the
   * single most useful mark on screen -- it pins the vertical framing, which
   * is what drifted between the two capture batches.
   */

  guide_dashes_h(GUIDE_ROI_X + 6, er, GUIDE_ROI_W - 12, COL_REF);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct v4l2_format fmt;
  struct fb_planeinfo_s pinfo;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  uintptr_t clean_start;
  uintptr_t clean_end;
  int seconds = 0;
  int vfd;
  int fbfd;
  long i;
  long loops;

  if (argc > 1)
    {
      seconds = atoi(argv[1]);
    }

  /* 1) Camera: 800x480 RGB565 straight into the LTDC framebuffer. */

  vfd = open("/dev/video0", O_RDONLY);
  if (vfd < 0)
    {
      printf("eye_guide: open /dev/video0 failed: %d\n", errno);
      return 1;
    }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = GUIDE_FRAME_W;
  fmt.fmt.pix.height      = GUIDE_FRAME_H;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;

  if (ioctl(vfd, VIDIOC_S_FMT, &fmt) < 0)
    {
      printf("eye_guide: VIDIOC_S_FMT failed: %d\n", errno);
      close(vfd);
      return 1;
    }

  /* 2) The buffer the panel scans out.  Camera writes it, we draw on it, the
   *    LTDC reads it -- one and the same memory.
   */

  fbfd = open("/dev/fb0", O_RDONLY);
  if (fbfd < 0)
    {
      printf("eye_guide: open /dev/fb0 failed: %d\n", errno);
      close(vfd);
      return 1;
    }

  if (ioctl(fbfd, FBIOGET_PLANEINFO, &pinfo) < 0)
    {
      printf("eye_guide: FBIOGET_PLANEINFO failed: %d\n", errno);
      close(fbfd);
      close(vfd);
      return 1;
    }

  g_fb        = (FAR uint16_t *)(uintptr_t)pinfo.fbmem;
  g_stride_px = pinfo.stride / 2;

  if (ioctl(vfd, VIDIOC_STREAMON, &type) < 0)
    {
      printf("eye_guide: VIDIOC_STREAMON failed: %d\n", errno);
      close(fbfd);
      close(vfd);
      return 1;
    }

  printf("eye_guide: ROI x=%d y=%d %dx%d, eye row y=%d, fb=%p stride=%u\n",
         GUIDE_ROI_X, GUIDE_ROI_Y, GUIDE_ROI_W, GUIDE_ROI_H,
         GUIDE_ROI_Y + GUIDE_EYE_ROW, (FAR void *)g_fb,
         (unsigned)pinfo.stride);
  printf("eye_guide: keep both eyes inside the green box, on the dashed line\n");
  printf("eye_guide: the box blinks a few times a second - that is normal\n");
  printf("eye_guide: for a steadier box: imx335_tune fps 5\n");

  /* Cache maintenance only covers the rows we draw on: the camera fills the
   * whole frame through DMA (no CPU cache involvement), so cleaning the ROI
   * band is enough and saves ~5x the cache work.
   */

  clean_start = (uintptr_t)(g_fb +
                (size_t)(GUIDE_ROI_Y - 4) * g_stride_px);
  clean_end   = (uintptr_t)(g_fb +
                (size_t)(GUIDE_ROI_Y + GUIDE_ROI_H + 4) * g_stride_px);

  /* 3) Redraw continuously: the camera overwrites whatever we draw within one
   *    frame time, so the guide has to be re-armed every few tens of ms.  The
   *    dcache clean is what makes it visible at all -- without it the LTDC
   *    keeps scanning stale memory (the r70 lesson).
   */

  loops = (seconds > 0) ? (long)seconds * (1000 / GUIDE_REDRAW_MS) : 0;
  for (i = 0; loops == 0 || i < loops; i++)
    {
      guide_draw();
      up_clean_dcache(clean_start, clean_end);
      usleep(GUIDE_REDRAW_MS * 1000);
    }

  ioctl(vfd, VIDIOC_STREAMOFF, &type);
  close(fbfd);
  close(vfd);

  printf("eye_guide: done\n");
  return 0;
}
