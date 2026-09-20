/****************************************************************************
 * apps/examples/palm_cam/palm_cam_main.c
 *
 * Palm detection on the ATON NPU with the camera as the input source:
 *
 *   IMX335 -> CSI-2 -> DCMIPP (RGB565 800x480, written straight into the LTDC
 *   framebuffer) -> CPU downscale to RGB888 192x192 -> ATON NPU (ST 995,
 *   033_palm_detection_full_quant_pc_uf_od) -> best palm detection.
 *
 * Tensor formats and the output map come from the generated model
 * (LL_ATON_*_Buffers_Info_Default in models/palm995/network.c), not from
 * guessing:
 *
 *   input   Input_0_out_0        uint8 RGB888, mem F{1,192,192,3} = 110592 B
 *                                (user allocated: the app owns the buffer)
 *
 *   output  Transpose_341_out_0  float32 F{1,2016,18} at +0
 *           Transpose_351_out_0  float32 F{1,2016,1}  at +145152 (scores)
 *
 * 995 has *no* user allocated output: the generated
 * LL_ATON_Set_User_Output_Buffer_Default() rejects every index, the two tensors
 * live inside the NPU pool, and the driver reports the region (153216 bytes at
 * 0x24223700) through the status ioctl.  The 18 values of an anchor are the
 * box and the 7 palm keypoints, so one head is decoded and printed.
 *
 * Console output goes through the ATON driver's polled raw channel (the
 * interrupt-driven console can lose its TX wakeup and block the writer).
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <math.h>

#include "palm_cam_anchors.h"

#include <sys/videoio.h>
#include <sys/ioctl.h>
#include <sys/mount.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>

#include <nuttx/arch.h>
#include <nuttx/cache.h>
#include <nuttx/aie/ai_engine.h>
#include <nuttx/aie/stm32n6_aton_aie.h>
#include <nuttx/video/fb.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PALM_CAM_INPUT_ADDR     0x243a2000   /* above the model pool's real high
                                              * water (0x34320000) and below the
                                              * NPU cache at 0x243c0000 */
#define PALM_CAM_OUTPUT_ADDR    0x24358000   /* fallback: only used when the
                                              * model owns a user output */

#define PALM_CAM_NN_W           192
#define PALM_CAM_NN_H           192
#define PALM_CAM_NN_C           3
#define PALM_CAM_IN_LEN         (PALM_CAM_NN_W * PALM_CAM_NN_H * PALM_CAM_NN_C)

#define PALM_CAM_ANCHORS        2016
#define PALM_CAM_HEAD_VALUES    18
#define PALM_CAM_HEAD_BYTES     (PALM_CAM_ANCHORS * PALM_CAM_HEAD_VALUES * 4)

/* 995 keeps its detector tensors inside the NPU pool - its generated
 * LL_ATON_Set_User_Output_Buffer_Default() refuses every index - so what the
 * runtime fills is one region that starts at the buffer base address:
 *
 *   Transpose_341_out_0  float32 F{1,2016,18}  [     0, 145152)
 *   Transpose_351_out_0  float32 F{1,2016,1}   [145152, 153216)
 *
 * i.e. 153216 bytes, which the driver reports as 0x24223700 (pool base
 * 0x24200000 plus the 145152 offset the descriptor spells out).
 */

#define PALM_CAM_SCORE_OFFSET   PALM_CAM_HEAD_BYTES            /* 145152 */
#define PALM_CAM_OUT_LEN        (PALM_CAM_SCORE_OFFSET + PALM_CAM_ANCHORS * 4)

#define PALM_CAM_FRAME_W        800
#define PALM_CAM_FRAME_H        480

#define PALM_CAM_CONF_THRESH    0.5f
#define PALM_CAM_INPUT_SIDE     192                /* model input side (px) */
#define PALM_CAM_FRAME_W        800                /* camera frame the box is
                                                    * scaled to              */
#define PALM_CAM_FRAME_H        480

/* Input geometry.  The model is fed a 5:3 picture with black rows underneath
 * (see CONFIG_EXAMPLES_PALM_CAM_LETTERBOX), so the picture occupies
 * PALM_CAM_INPUT_ROWS of the 192x192 input and both axes are scaled by the
 * same 800/192 ratio.  A coordinate in the input space therefore maps back to
 * the camera frame by that single ratio - the stretched layout would need a
 * different one per axis, which is why the ratio is a macro here.
 */

#if defined(CONFIG_EXAMPLES_PALM_CAM_LETTERBOX)
#  define PALM_CAM_INPUT_ROWS   ((PALM_CAM_NN_H * PALM_CAM_FRAME_H) / \
                                 PALM_CAM_FRAME_W)
#  define PALM_CAM_INPUT_Y_SCALE ((float)PALM_CAM_FRAME_W / \
                                  (float)PALM_CAM_INPUT_SIDE)
#else
#  define PALM_CAM_INPUT_ROWS   PALM_CAM_NN_H
#  define PALM_CAM_INPUT_Y_SCALE ((float)PALM_CAM_FRAME_H / \
                                  (float)PALM_CAM_INPUT_SIDE)
#endif

#define PALM_CAM_INPUT_X_SCALE  ((float)PALM_CAM_FRAME_W / \
                                 (float)PALM_CAM_INPUT_SIDE)

#ifndef CONFIG_EXAMPLES_PALM_CAM_MOUNTPOINT
#  define CONFIG_EXAMPLES_PALM_CAM_MOUNTPOINT "/mnt/sdcard"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static int g_palm_cam_fd = -1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void palm_cam_out(FAR const char *fmt, ...)
{
  char buf[160];
  va_list ap;
  int n;

  va_start(ap, fmt);
  n = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  if (n <= 0)
    {
      return;
    }

  if (g_palm_cam_fd >= 0)
    {
      (void)ioctl(g_palm_cam_fd, STM32N6_ATON_CMD_PUTS, (unsigned long)buf);
    }
  else
    {
      fputs(buf, stdout);
      fflush(stdout);
    }
}

static void palm_cam_fixed(FAR char *buf, size_t n, float v)
{
  bool neg = false;
  long scaled;

  if (v < 0.0f)
    {
      neg = true;
      v = -v;
    }

  scaled = (long)(v * 100.0f + 0.5f);
  snprintf(buf, n, "%s%ld.%02ld", neg ? "-" : "", scaled / 100, scaled % 100);
}

/* RGB565 800x480 (LTDC framebuffer) -> RGB888 192x192 model input, box
 * average over the source block.
 */

static void palm_cam_frame_to_input(FAR const uint16_t *fb, uint32_t stride,
                                    FAR uint8_t *dst)
{
  uint32_t stride_px = stride / 2;
  int x;
  int y;

#ifdef CONFIG_EXAMPLES_PALM_CAM_LETTERBOX
  /* ST reference layout: a 192x115 picture (192 * 480/800, the camera aspect
   * ratio) at the top of the input and 77 black rows underneath.  The row
   * mapping below already produces the 480 -> 115 band, so only the padding
   * has to be written here.
   */

  memset(dst, 0, PALM_CAM_IN_LEN);
#endif

  for (y = 0; y < PALM_CAM_INPUT_ROWS; y++)
    {
      uint32_t sy0 = (uint32_t)y * PALM_CAM_FRAME_H / PALM_CAM_INPUT_ROWS;
      uint32_t sy1 = (uint32_t)(y + 1) * PALM_CAM_FRAME_H /
                     PALM_CAM_INPUT_ROWS;
      uint32_t sy;

      if (sy1 <= sy0)
        {
          sy1 = sy0 + 1;
        }

      for (x = 0; x < PALM_CAM_NN_W; x++)
        {
          uint32_t sx0 = (uint32_t)x * PALM_CAM_FRAME_W / PALM_CAM_NN_W;
          uint32_t sx1 = (uint32_t)(x + 1) * PALM_CAM_FRAME_W / PALM_CAM_NN_W;
          uint32_t rsum = 0;
          uint32_t gsum = 0;
          uint32_t bsum = 0;
          uint32_t n = 0;
          uint32_t sx;

          if (sx1 <= sx0)
            {
              sx1 = sx0 + 1;
            }

          for (sy = sy0; sy < sy1 && sy < PALM_CAM_FRAME_H; sy++)
            {
              FAR const uint16_t *row = fb + sy * stride_px;

              for (sx = sx0; sx < sx1 && sx < PALM_CAM_FRAME_W; sx++)
                {
                  uint16_t p = row[sx];

                  rsum += (p >> 11) & 0x1f;
                  gsum += (p >> 5) & 0x3f;
                  bsum += p & 0x1f;
                  n++;
                }
            }

          if (n == 0)
            {
              n = 1;
            }

          {
            uint32_t r = rsum / n;
            uint32_t g = gsum / n;
            uint32_t b = bsum / n;

            *dst++ = (uint8_t)((r << 3) | (r >> 2));
            *dst++ = (uint8_t)((g << 2) | (g >> 4));
            *dst++ = (uint8_t)((b << 3) | (b >> 2));
          }
        }
    }
}

/* One float tensor: min / max / average, printed as fixed point. */

static void palm_cam_tensor_stats(FAR const char *tag,
                                  FAR const float *p, int count)
{
  float lo = 1.0e30f;
  float hi = -1.0e30f;
  float avg = 0.0f;
  char sa[24];
  char sb[24];
  char sc[24];
  int i;

  for (i = 0; i < count; i++)
    {
      float v = p[i];

      if (v < lo)
        {
          lo = v;
        }

      if (v > hi)
        {
          hi = v;
        }

      avg += v;
    }

  avg /= (float)count;
  palm_cam_fixed(sa, sizeof(sa), lo);
  palm_cam_fixed(sb, sizeof(sb), hi);
  palm_cam_fixed(sc, sizeof(sc), avg);
  palm_cam_out("palm_cam: %s min %s max %s avg %s\n", tag, sa, sb, sc);
}

/* Best anchor in the score tensor, then the 18 values of that anchor from the
 * declared head: 4 box values followed by 7 keypoints.
 */

/* Anchor grid of this model: 2016 anchors = (24x24 + 12x12 + 6x6) x 2, i.e.
 * strides 8/16/32 with two anchors per cell.  The order that reproduces this
 * count is "stride, then row, then column, then the two anchors", which is what
 * the index arithmetic below implements.  Without it the printout is a
 * regression value, not a coordinate.
 */

static void palm_cam_anchor(int idx, FAR float *nx, FAR float *ny)
{
  *nx = g_palm_anchors[idx][0];
  *ny = g_palm_anchors[idx][1];
}

/* -------------------------------------------------------------------
 * LCD overlay
 *
 * The LTDC framebuffer exposes a single plane and the DCMIPP writes the live
 * picture into that very buffer, so anything drawn while the camera streams
 * is overwritten within one frame time (~33 ms).  The caller therefore stops
 * the stream as soon as a frame has been captured and then annotates the
 * frozen picture: a green/red band across the top, the confidence as
 * digits and a bar, the decoded box and the seven keypoints.  All of it
 * stays on the LCD after the program exits, so the result is readable
 * without a serial console.
 * -------------------------------------------------------------------
 */

#define PALM_CAM_FB_BPP     2          /* RGB565 */
#define PALM_CAM_COL_FOUND  0x07e0     /* green  */
#define PALM_CAM_COL_MISS   0xf800     /* red    */
#define PALM_CAM_COL_KP     0xffe0     /* yellow */
#define PALM_CAM_COL_TEXT   0xffff     /* white  */
#define PALM_CAM_COL_DIM    0x8410     /* grey   */

static uint16_t *g_palm_fb;           /* LTDC background buffer (RGB565) */
static uint32_t g_palm_fb_stride_px;  /* pixels per line */

static void palm_cam_fb_hline(int x, int y, int len, uint16_t color)
{
  FAR uint16_t *row;

  if (g_palm_fb == NULL || len <= 0 || y < 0 || y >= PALM_CAM_FRAME_H)
    {
      return;
    }

  if (x < 0)
    {
      len += x;
      x = 0;
    }

  if (x + len > PALM_CAM_FRAME_W)
    {
      len = PALM_CAM_FRAME_W - x;
    }

  if (len <= 0)
    {
      return;
    }

  /* Written straight into the buffer the LTDC scans out: going through
   * write() would need the framebuffer chardev to hand out the same memory,
   * and this app already holds the pointer it reads the frame from.
   */

  row = g_palm_fb + (size_t)y * g_palm_fb_stride_px + (size_t)x;

  while (len-- > 0)
    {
      *row++ = color;
    }
}

static void palm_cam_fb_vline(int x, int y, int len, uint16_t color)
{
  int i;

  for (i = 0; i < len; i++)
    {
      palm_cam_fb_hline(x, y + i, 1, color);
    }
}

static void palm_cam_fb_rect(int x, int y, int w, int h, int t,
                             uint16_t color)
{
  int i;

  if (w <= 0 || h <= 0)
    {
      return;
    }

  for (i = 0; i < t; i++)
    {
      palm_cam_fb_hline(x, y + i, w, color);
      palm_cam_fb_hline(x, y + h - 1 - i, w, color);
      palm_cam_fb_vline(x + i, y, h, color);
      palm_cam_fb_vline(x + w - 1 - i, y, h, color);
    }
}

/* Confidence digits: a 5x7 bitmap font, column major, bit0 = top row.  Only
 * the characters the annotation needs are kept, so the table stays tiny.
 */

static FAR const uint8_t *palm_cam_fb_glyph(char ch)
{
  static const uint8_t digits[10][5] =
  {
    {0x3e, 0x51, 0x49, 0x45, 0x3e},   /* 0 */
    {0x00, 0x42, 0x7f, 0x40, 0x00},   /* 1 */
    {0x42, 0x61, 0x51, 0x49, 0x46},   /* 2 */
    {0x21, 0x41, 0x45, 0x4b, 0x31},   /* 3 */
    {0x18, 0x14, 0x12, 0x7f, 0x10},   /* 4 */
    {0x27, 0x45, 0x45, 0x45, 0x39},   /* 5 */
    {0x3c, 0x4a, 0x49, 0x49, 0x30},   /* 6 */
    {0x01, 0x71, 0x09, 0x05, 0x03},   /* 7 */
    {0x36, 0x49, 0x49, 0x49, 0x36},   /* 8 */
    {0x06, 0x49, 0x49, 0x29, 0x1e}    /* 9 */
  };

  static const uint8_t dot[5]   = {0x00, 0x60, 0x60, 0x00, 0x00};
  static const uint8_t dash[5]  = {0x08, 0x08, 0x08, 0x08, 0x08};
  static const uint8_t blank[5] = {0x00, 0x00, 0x00, 0x00, 0x00};

  if (ch >= '0' && ch <= '9')
    {
      return digits[ch - '0'];
    }

  if (ch == '.')
    {
      return dot;
    }

  if (ch == '-')
    {
      return dash;
    }

  return blank;
}

static void palm_cam_fb_text(int x, int y, FAR const char *s, int scale,
                             uint16_t color)
{
  int col;
  int row;

  for (; *s != '\0'; s++)
    {
      FAR const uint8_t *g = palm_cam_fb_glyph(*s);

      for (col = 0; col < 5; col++)
        {
          for (row = 0; row < 7; row++)
            {
              int i;

              if ((g[col] & (1u << row)) == 0)
                {
                  continue;
                }

              for (i = 0; i < scale; i++)
                {
                  palm_cam_fb_hline(x + col * scale, y + row * scale + i,
                                    scale, color);
                }
            }
        }

      x += 6 * scale;
    }
}

static void palm_cam_overlay_draw(FAR const float *box, FAR const float *kps,
                                  float logit)
{
  float prob = 1.0f / (1.0f + expf(-logit));
  uint16_t col;
  int hundred = (int)(prob * 100.0f + 0.5f);
  char buf[12];
  int x = (int)box[0];
  int y = (int)box[1];
  int w = (int)box[2];
  int h = (int)box[3];
  int bar;
  int k;

  if (g_palm_fb == NULL)
    {
      palm_cam_out("palm_cam: overlay skipped (no framebuffer pointer)\n");
      return;
    }

  col = (logit > 0.0f) ? PALM_CAM_COL_FOUND : PALM_CAM_COL_MISS;

  if (hundred > 100)
    {
      hundred = 100;
    }

  snprintf(buf, sizeof(buf), "%d.%02d", hundred / 100, hundred % 100);

  /* A band across the top: green when the palm was found, red otherwise.  It
   * is the one thing that can be read from a metre away.
   */

  palm_cam_fb_hline(0, 0, PALM_CAM_FRAME_W, col);
  palm_cam_fb_hline(0, 1, PALM_CAM_FRAME_W, col);
  palm_cam_fb_hline(0, 2, PALM_CAM_FRAME_W, col);

  /* Confidence, four times the font size, plus a bar of the same value. */

  palm_cam_fb_text(8, 12, buf, 4, PALM_CAM_COL_TEXT);

  bar = PALM_CAM_FRAME_W / 4;
  palm_cam_fb_rect(8, 44, bar, 12, 1, PALM_CAM_COL_TEXT);

  for (k = 0; k < (int)((float)(bar - 4) * prob + 0.5f); k++)
    {
      palm_cam_fb_vline(10 + k, 46, 10, col);
    }

  /* The decoded box and the seven keypoints, in frame coordinates. */

  palm_cam_fb_rect(x, y, w, h, 2, col);

  for (k = 0; k < 7; k++)
    {
      palm_cam_fb_rect((int)kps[k * 2 + 0] - 3, (int)kps[k * 2 + 1] - 3,
                       7, 7, 1, PALM_CAM_COL_KP);
    }

  /* The framebuffer lives in cacheable SRAM and the LTDC reads it through its
   * own port, so the lines above have to leave the D-cache before the panel
   * can show them; without this the screen keeps the old picture and the
   * overlay looks like it was never drawn.
   */

  up_clean_dcache((uintptr_t)g_palm_fb,
                  (uintptr_t)g_palm_fb +
                  (size_t)PALM_CAM_FRAME_H * g_palm_fb_stride_px * 2);

  palm_cam_out("palm_cam: overlay drawn (box %d,%d %dx%d, prob %d.%02d)\n",
               x, y, w, h, hundred / 100, hundred % 100);
}

static void palm_cam_decode(FAR const float *out)
{
  FAR const float *scores = out + PALM_CAM_SCORE_OFFSET / 4;
  FAR const float *v;
  float best = -1.0e30f;
  int best_a = -1;
  int i;

  for (i = 0; i < PALM_CAM_ANCHORS; i++)
    {
      if (scores[i] > best)
        {
          best = scores[i];
          best_a = i;
        }
    }

  {
    char sa[24];
    char sb2[24];
    int above = 0;

    /* The model emits LOGITS, so the confidence threshold has to be applied to
     * the sigmoid, not to the raw value: r57 compared the logit against 0.50
     * and therefore always reported "0 anchor(s) above 0.50", even for frames
     * whose peak logit was +3.0 (probability 0.95).
     */

    for (i = 0; i < PALM_CAM_ANCHORS; i++)
      {
        if (1.0f / (1.0f + expf(-scores[i])) >= PALM_CAM_CONF_THRESH)
          {
            above++;
          }
      }

    palm_cam_fixed(sa, sizeof(sa), best);
    palm_cam_fixed(sb2, sizeof(sb2), 1.0f / (1.0f + expf(-best)));
    palm_cam_out("palm_cam: best palm logit %s (prob %s) at anchor %d, "
                 "%d anchor(s) with prob >= 0.50\n", sa, sb2, best_a, above);

    /* One line the caller can grep for.  A peak logit at or below zero puts
     * the sigmoid at 0.50 or less, i.e. the winning anchor sits at the level
     * the network reserves for background; without this verdict an empty
     * frame still reported "prob 0.50 at anchor 0" and looked like a weak
     * detection.
     */

    if (best <= 0.0f)
      {
        palm_cam_out("palm_cam: verdict no palm (peak logit %s <= 0.00, so no "
                     "anchor reaches prob 0.50: every candidate is at or "
                     "below the background level)\n", sa);
      }
    else
      {
        palm_cam_out("palm_cam: verdict palm (peak logit %s > 0.00, prob %s, "
                     "anchor %d)\n", sa, sb2, best_a);
      }

    /* Top three: a frame with several palm candidates still shows which one
     * won and by how much (one pass, three locals - no extra buffer).
     */

    {
      float b0 = -1.0e30f;
      float b1 = -1.0e30f;
      float b2 = -1.0e30f;
      int i0 = -1;
      int i1 = -1;
      int i2 = -1;

      for (i = 0; i < PALM_CAM_ANCHORS; i++)
        {
          if (scores[i] > b0)
            {
              b2 = b1; i2 = i1;
              b1 = b0; i1 = i0;
              b0 = scores[i]; i0 = i;
            }
          else if (scores[i] > b1)
            {
              b2 = b1; i2 = i1;
              b1 = scores[i]; i1 = i;
            }
          else if (scores[i] > b2)
            {
              b2 = scores[i]; i2 = i;
            }
        }

      {
        char p1[24];
        char p2[24];

        palm_cam_fixed(p1, sizeof(p1), 1.0f / (1.0f + expf(-b0)));
        palm_cam_fixed(p2, sizeof(p2), 1.0f / (1.0f + expf(-b1)));
        palm_cam_out("palm_cam: top anchors %d (prob %s) %d (prob %s)\n",
                     i0, p1, i1, p2);
      }
    }

    if (scores == NULL)
      {
        return;
      }
  

    {
      float nx;
      float ny;
      char sa2[24];
      char sb3[24];
      char sc2[24];
      char sd2[24];

      palm_cam_anchor(best_a, &nx, &ny);
      palm_cam_fixed(sa2, sizeof(sa2), nx);
      palm_cam_fixed(sb3, sizeof(sb3), ny);
      palm_cam_fixed(sc2, sizeof(sc2), nx * (float)PALM_CAM_INPUT_SIDE);
      palm_cam_fixed(sd2, sizeof(sd2), ny * (float)PALM_CAM_INPUT_SIDE);
      palm_cam_out("palm_cam: anchor %d normalised (%s,%s) px (%s,%s)\n",
                   best_a, sa2, sb3, sc2, sd2);
    }
  }

  v = out + (size_t)best_a * PALM_CAM_HEAD_VALUES;

  {
    char sa[24];
    char sb[24];
    char sc[24];
    char sd[24];
    int k;

    palm_cam_fixed(sa, sizeof(sa), v[0]);
    palm_cam_fixed(sb, sizeof(sb), v[1]);
    palm_cam_fixed(sc, sizeof(sc), v[2]);
    palm_cam_fixed(sd, sizeof(sd), v[3]);
    palm_cam_out("palm_cam: head box %s %s %s %s (anchor %d)\n",
                 sa, sb, sc, sd, best_a);

    for (k = 0; k < 7; k++)
      {
        palm_cam_fixed(sa, sizeof(sa), v[4 + k * 2 + 0]);
        palm_cam_fixed(sb, sizeof(sb), v[4 + k * 2 + 1]);
        palm_cam_out("palm_cam: kp[%d] x=%s y=%s\n", k, sa, sb);
      }

    /* Decoded view: the raw numbers are a centre/size regression plus keypoint
     * offsets from that centre, in the 192x192 input space.  Both the input
     * space and the camera frame are printed so the box can be checked against
     * palm_frame.ppm.
     */

    {
      float nx;
      float ny;
      float cx;
      float cy;
      float bw = v[2];
      float bh = v[3];

      /* ST reference decode (Middlewares/STM32_VISION_MODELS_PP/Src/
       * pd_pp_model.c): both the box centre and every keypoint are the anchor
       * centre plus the model's raw pixel offset, so the anchor table is not
       * optional - without it every coordinate is off by up to half the image.
       */

      palm_cam_anchor(best_a, &nx, &ny);
      cx = nx * (float)PALM_CAM_INPUT_SIDE + v[0];
      cy = ny * (float)PALM_CAM_INPUT_SIDE + v[1];
      char s5[24];
      char s6[24];
      char s7[24];
      char s8[24];

      palm_cam_fixed(sa, sizeof(sa), cx - bw * 0.5f);
      palm_cam_fixed(sb, sizeof(sb), cy - bh * 0.5f);
      palm_cam_fixed(sc, sizeof(sc), bw);
      palm_cam_fixed(sd, sizeof(sd), bh);
      palm_cam_out("palm_cam: decoded box (192) x=%s y=%s w=%s h=%s\n",
                   sa, sb, sc, sd);

      palm_cam_fixed(s5, sizeof(s5),
                     (cx - bw * 0.5f) * PALM_CAM_INPUT_X_SCALE);
      palm_cam_fixed(s6, sizeof(s6),
                     (cy - bh * 0.5f) * PALM_CAM_INPUT_Y_SCALE);
      palm_cam_fixed(s7, sizeof(s7), bw * PALM_CAM_INPUT_X_SCALE);
      palm_cam_fixed(s8, sizeof(s8), bh * PALM_CAM_INPUT_Y_SCALE);
      palm_cam_out("palm_cam: decoded box (frame) x=%s y=%s w=%s h=%s\n",
                   s5, s6, s7, s8);

      for (k = 0; k < 7; k++)
        {
          float kx = nx * (float)PALM_CAM_INPUT_SIDE + v[4 + k * 2 + 0];
          float ky = ny * (float)PALM_CAM_INPUT_SIDE + v[4 + k * 2 + 1];

          palm_cam_fixed(sa, sizeof(sa), kx);
          palm_cam_fixed(sb, sizeof(sb), ky);
          palm_cam_fixed(sc, sizeof(sc), kx * PALM_CAM_INPUT_X_SCALE);
          palm_cam_fixed(sd, sizeof(sd), ky * PALM_CAM_INPUT_Y_SCALE);
          palm_cam_out("palm_cam: decoded kp[%d] (%s,%s) frame (%s,%s)\n",
                       k, sa, sb, sc, sd);
        }

      /* Annotate the frozen frame on the LCD.  The same frame-space mapping as
       * the lines above is used, so the rectangle on screen belongs to the
       * camera picture and not to the 192x192 input space.
       */

      {
        float box[4];
        float kpf[14];

        box[0] = (cx - bw * 0.5f) * PALM_CAM_INPUT_X_SCALE;
        box[1] = (cy - bh * 0.5f) * PALM_CAM_INPUT_Y_SCALE;
        box[2] = bw * PALM_CAM_INPUT_X_SCALE;
        box[3] = bh * PALM_CAM_INPUT_Y_SCALE;

        for (k = 0; k < 7; k++)
          {
            kpf[k * 2 + 0] = (nx * (float)PALM_CAM_INPUT_SIDE +
                              v[4 + k * 2 + 0]) * PALM_CAM_INPUT_X_SCALE;
            kpf[k * 2 + 1] = (ny * (float)PALM_CAM_INPUT_SIDE +
                              v[4 + k * 2 + 1]) * PALM_CAM_INPUT_Y_SCALE;
          }

        palm_cam_overlay_draw(box, kpf, best);
      }
    }
  }
}

/* r51 P3: the tensor statistics print two decimals, which cannot tell +0.0
 * (0x00000000) from -0.0 (0x80000000) or from a denormal - and those are
 * exactly the interesting cases here.  Print the first words of both tensors
 * raw, plus how much of the region is non-zero at all.
 */

static void palm_cam_raw_report(FAR const uint8_t *out, size_t len)
{
  FAR const uint32_t *w = (FAR const uint32_t *)out;
  char line[200];
  size_t i;
  size_t nz = 0;
  size_t first = len;
  int n;
  int k;

  for (i = 0; i < len; i++)
    {
      if (out[i] != 0)
        {
          nz++;
          if (first == len)
            {
              first = i;
            }
        }
    }

  n = 0;
  for (k = 0; k < 8; k++)
    {
      n += snprintf(line + n, sizeof(line) - (size_t)n, " %08lx",
                    (unsigned long)w[k]);
    }

  palm_cam_out("palm_cam: raw head[0..7]:%s\n", line);

  n = 0;
  for (k = 0; k < 8; k++)
    {
      n += snprintf(line + n, sizeof(line) - (size_t)n, " %08lx",
                    (unsigned long)w[PALM_CAM_SCORE_OFFSET / 4 + k]);
    }

  palm_cam_out("palm_cam: raw score[0..7]:%s\n", line);
  palm_cam_out("palm_cam: nonzero bytes %lu/%lu, first non-zero at %lu\n",
               (unsigned long)nz, (unsigned long)len, (unsigned long)first);
}

static void palm_cam_checksum(FAR const uint8_t *buf, size_t len,
                              FAR uint32_t *ck, FAR uint8_t *minv,
                              FAR uint8_t *maxv)
{
  uint32_t c = 0;
  uint8_t lo = 0xff;
  uint8_t hi = 0x00;
  size_t i;

  for (i = 0; i < len; i++)
    {
      uint8_t v = buf[i];

      c = (c << 1) ^ v;
      if (v < lo)
        {
          lo = v;
        }

      if (v > hi)
        {
          hi = v;
        }
    }

  *ck = c;
  *minv = lo;
  *maxv = hi;
}

static void palm_cam_save(FAR const uint8_t *input, size_t in_len,
                          FAR const uint8_t *output, size_t out_len,
                          FAR const float *out)
{
  FAR const char *mnt = CONFIG_EXAMPLES_PALM_CAM_MOUNTPOINT;
  FAR const float *scores = out + PALM_CAM_SCORE_OFFSET / 4;
  char path[128];
  char a[24];
  FILE *fp;
  float best = -1.0e30f;
  int best_a = -1;
  int i;

  for (i = 0; i < PALM_CAM_ANCHORS; i++)
    {
      if (scores[i] > best)
        {
          best = scores[i];
          best_a = i;
        }
    }

  snprintf(path, sizeof(path), "%s/palm_in.bin", mnt);
  fp = fopen(path, "wb");
  if (fp == NULL)
    {
      palm_cam_out("palm_cam: cannot write %s: %d (mount the card?)\n",
                   path, errno);
      return;
    }

  fwrite(input, 1, in_len, fp);
  fclose(fp);
  palm_cam_out("palm_cam: saved %s (%zu bytes)\n", path, in_len);

  snprintf(path, sizeof(path), "%s/palm_out.bin", mnt);
  fp = fopen(path, "wb");
  if (fp != NULL)
    {
      fwrite(output, 1, out_len, fp);
      fclose(fp);
      palm_cam_out("palm_cam: saved %s (%zu bytes)\n", path, out_len);
    }

  snprintf(path, sizeof(path), "%s/palm_frame.ppm", mnt);
  fp = fopen(path, "wb");
  if (fp != NULL)
    {
      fprintf(fp, "P6\n%d %d\n255\n", PALM_CAM_NN_W, PALM_CAM_NN_H);
      fwrite(input, 1, (size_t)PALM_CAM_NN_W * PALM_CAM_NN_H * 3, fp);
      fclose(fp);
      palm_cam_out("palm_cam: saved %s\n", path);
    }

  snprintf(path, sizeof(path), "%s/palm_kpts.txt", mnt);
  fp = fopen(path, "wb");
  if (fp != NULL)
    {
      palm_cam_fixed(a, sizeof(a), best);
      fprintf(fp, "# score %s anchor %d\n", a, best_a);
      fprintf(fp, "# %d %d %d\n", 7, PALM_CAM_NN_W, PALM_CAM_NN_H);

      for (i = 0; i < 7; i++)
        {
          char b[24];

          palm_cam_fixed(a, sizeof(a), out[best_a * PALM_CAM_HEAD_VALUES + 4 + i * 2]);
          palm_cam_fixed(b, sizeof(b), out[best_a * PALM_CAM_HEAD_VALUES + 4 + i * 2 + 1]);
          fprintf(fp, "%02d %s %s 1.00\n", i, a, b);
        }

      fclose(fp);
      palm_cam_out("palm_cam: saved %s\n", path);
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct stm32n6_aton_invoke_params params;
  struct stm32n6_aton_status_s status;
  struct v4l2_format fmt;
  struct fb_planeinfo_s pinfo;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  FAR uint8_t *input  = (FAR uint8_t *)PALM_CAM_INPUT_ADDR;
  FAR uint8_t *output = (FAR uint8_t *)PALM_CAM_OUTPUT_ADDR;
  FAR const float *fout = (FAR const float *)PALM_CAM_OUTPUT_ADDR;
  FAR uint16_t *fb;
  FAR const char *replay = NULL;
  uint32_t ck = 0;
  uint32_t ck2 = 0;
  uint8_t minv;
  uint8_t maxv;
  int runs = 1;
  int vfd = -1;
  int fbfd = -1;
  int aiefd = -1;
  int luma = 0;
  int ret;
  int i;

  if (argc > 1)
    {
      runs = atoi(argv[1]);
      if (runs < 1)
        {
          runs = 1;
        }
    }

  /* Optional second argument: replay a saved model input (110592 bytes) through
   * the NPU instead of converting a captured frame.  Feeding the exact bytes
   * that the PC reference implementation consumed is what turns a
   * board-vs-reference comparison into a single variable experiment.
   */

  if (argc > 2 && argv[2] != NULL && argv[2][0] != '\0')
    {
      replay = argv[2];
    }

  aiefd = open("/dev/aie0", O_RDWR);
  if (aiefd < 0)
    {
      fprintf(stderr, "palm_cam: open /dev/aie0 failed: %d\n", errno);
      return 1;
    }

  g_palm_cam_fd = aiefd;

  palm_cam_out("palm_cam: camera -> ATON NPU (ST 995 palm detection)\n");

  ret = ioctl(aiefd, AIE_CMD_LOAD, 0);
  if (ret < 0)
    {
      palm_cam_out("palm_cam: AIE_CMD_LOAD failed: %d\n", errno);
      close(aiefd);
      return 1;
    }

  memset(&status, 0, sizeof(status));
  ret = ioctl(aiefd, STM32N6_ATON_CMD_GET_STATUS, (unsigned long)&status);
  if (ret < 0)
    {
      palm_cam_out("palm_cam: GET_STATUS failed: %d\n", errno);
      close(aiefd);
      return 1;
    }

  palm_cam_out("palm_cam: model input %lu bytes (want %d), output %lu bytes "
               "(want %d)\n",
               (unsigned long)status.input_size, PALM_CAM_IN_LEN,
               (unsigned long)status.output_size, PALM_CAM_OUT_LEN);

  if (status.input_size != PALM_CAM_IN_LEN ||
      status.output_size != PALM_CAM_OUT_LEN)
    {
      palm_cam_out("palm_cam: refusing to run: the tensor sizes do not match "
                   "this test (want %d/%d)\n",
                   PALM_CAM_IN_LEN, PALM_CAM_OUT_LEN);
      close(aiefd);
      return 1;
    }

  /* The palm model has no user allocated output: the runtime writes straight
   * into the NPU pool and the driver reports where.  A model with a user
   * output reports 0 here and this app's own buffer is used instead.
   */

  if (status.output_addr != 0)
    {
      output = (FAR uint8_t *)(uintptr_t)status.output_addr;
      fout   = (FAR const float *)(uintptr_t)status.output_addr;
      palm_cam_out("palm_cam: output region 0x%08lx, %lu bytes (head then "
                   "scores), inside the NPU pool\n",
                   (unsigned long)status.output_addr,
                   (unsigned long)status.output_size);
    }

  memset(input, 0, status.input_size);

  vfd = open("/dev/video0", O_RDONLY);
  if (vfd < 0)
    {
      palm_cam_out("palm_cam: open /dev/video0 failed: %d\n", errno);
      close(aiefd);
      return 1;
    }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = PALM_CAM_FRAME_W;
  fmt.fmt.pix.height      = PALM_CAM_FRAME_H;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;

  if (ioctl(vfd, VIDIOC_S_FMT, &fmt) < 0)
    {
      palm_cam_out("palm_cam: VIDIOC_S_FMT failed: %d\n", errno);
      close(vfd);
      close(aiefd);
      return 1;
    }

  if (ioctl(vfd, VIDIOC_STREAMON, &type) < 0)
    {
      palm_cam_out("palm_cam: VIDIOC_STREAMON failed: %d\n", errno);
      close(vfd);
      close(aiefd);
      return 1;
    }

  fbfd = open("/dev/fb0", O_RDONLY);
  if (fbfd < 0)
    {
      palm_cam_out("palm_cam: open /dev/fb0 failed: %d\n", errno);
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      close(vfd);
      close(aiefd);
      return 1;
    }

  if (ioctl(fbfd, FBIOGET_PLANEINFO, &pinfo) < 0)
    {
      palm_cam_out("palm_cam: FBIOGET_PLANEINFO failed: %d\n", errno);
      close(fbfd);
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      close(vfd);
      close(aiefd);
      return 1;
    }

  fb = (FAR uint16_t *)(uintptr_t)pinfo.fbmem;

#ifdef CONFIG_EXAMPLES_PALM_CAM_OVERLAY
  /* The overlay is drawn straight into this pointer: it is the same memory the
   * camera fills and this app reads the frame from.  The assignment has to sit
   * after the ioctl *and* after `fb` itself - r71 had it earlier, so the
   * pointer was still NULL and every draw call returned without a word.
   */

  g_palm_fb = fb;
  g_palm_fb_stride_px = pinfo.stride / 2;

  palm_cam_out("palm_cam: overlay target %p, stride %u, %ux%u\n",
               (FAR void *)fb, (unsigned)pinfo.stride,
               PALM_CAM_FRAME_W, PALM_CAM_FRAME_H);
#endif

  if (replay != NULL)
    {
      int rfd = open(replay, O_RDONLY);
      size_t got = 0;

      if (rfd < 0)
        {
          palm_cam_out("palm_cam: open %s failed: %d\n", replay, errno);
          close(fbfd);
          ioctl(vfd, VIDIOC_STREAMOFF, &type);
          close(vfd);
          close(aiefd);
          return 1;
        }

      while (got < (size_t)status.input_size)
        {
          ssize_t n = read(rfd, input + got, status.input_size - got);

          if (n <= 0)
            {
              break;
            }

          got += (size_t)n;
        }

      close(rfd);
      palm_cam_out("palm_cam: replay %s: %u of %lu bytes loaded\n", replay,
                   (unsigned)got, (unsigned long)status.input_size);

      if (got != (size_t)status.input_size)
        {
          palm_cam_out("palm_cam: replay short read, refusing to run\n");
          close(fbfd);
          ioctl(vfd, VIDIOC_STREAMOFF, &type);
          close(vfd);
          close(aiefd);
          return 1;
        }
    }
  else
    {
      if (read(vfd, NULL, 0) < 0)
        {
          palm_cam_out("palm_cam: frame sync failed: %d\n", errno);
          close(fbfd);
          ioctl(vfd, VIDIOC_STREAMOFF, &type);
          close(vfd);
          close(aiefd);
          return 1;
        }

#ifdef CONFIG_EXAMPLES_PALM_CAM_OVERLAY
      /* Freeze right here: the overlay is drawn once the inference is done,
       * and the DCMIPP writes the same buffer the LTDC scans out, so a frame
       * arriving in the meantime would erase it before it is ever seen.
       */

      ioctl(vfd, VIDIOC_STREAMOFF, &type);
#endif

      palm_cam_frame_to_input(fb, pinfo.stride, input);
    }

  for (i = 0; i < PALM_CAM_IN_LEN; i += 3)
    {
      luma += input[i];
    }

  luma = luma / (PALM_CAM_IN_LEN / 3);

#ifdef CONFIG_EXAMPLES_PALM_CAM_LETTERBOX
  palm_cam_out("palm_cam: frame %dx%d -> input %dx%d (letterbox: %dx%d "
               "picture, %d black rows), mean luma %d\n",
               PALM_CAM_FRAME_W, PALM_CAM_FRAME_H,
               PALM_CAM_NN_W, PALM_CAM_NN_H, PALM_CAM_NN_W,
               PALM_CAM_INPUT_ROWS, PALM_CAM_NN_H - PALM_CAM_INPUT_ROWS, luma);
#else
  palm_cam_out("palm_cam: frame %dx%d -> input %dx%d (stretched: aspect ratio "
               "not kept), mean luma %d\n",
               PALM_CAM_FRAME_W, PALM_CAM_FRAME_H,
               PALM_CAM_NN_W, PALM_CAM_NN_H, luma);
#endif

  up_clean_dcache((uintptr_t)input, (uintptr_t)input + status.input_size);

  for (i = 0; i < runs; i++)
    {
      memset(&params, 0, sizeof(params));
      params.input       = input;
      params.output      = output;
      params.input_size  = status.input_size;
      params.output_size = status.output_size;

      memset(output, 0, status.output_size);

      ret = ioctl(aiefd, AIE_CMD_FEED_INPUT, (unsigned long)&params);
      if (ret < 0)
        {
          palm_cam_out("palm_cam: AIE_CMD_FEED_INPUT failed: %d\n", errno);
          break;
        }

      up_invalidate_dcache((uintptr_t)output,
                           (uintptr_t)output + status.output_size);
      palm_cam_checksum(output, status.output_size, &ck, &minv, &maxv);

      palm_cam_out("palm_cam: run %d/%d ok: %lu us, %lu events, result %d, "
                   "checksum 0x%08lx (bytes min %u max %u)\n",
                   i + 1, runs, (unsigned long)params.usec,
                   (unsigned long)params.events, params.result,
                   (unsigned long)ck, (unsigned)minv, (unsigned)maxv);

      if (i == 1 && ck != ck2)
        {
          palm_cam_out("palm_cam: WARNING same frame, different checksum\n");
        }

      ck2 = ck;
    }

  if (i == 0)
    {
      close(fbfd);
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      close(vfd);
      close(aiefd);
      return 1;
    }

  /* r51 P3: raw view of the output before any decoding. */

  palm_cam_raw_report(output, status.output_size);

  /* Both output tensors, so the map from the descriptor is visible in the data
   * before anything is decoded.
   */

  palm_cam_tensor_stats("head[2016x18]", fout,
                        PALM_CAM_ANCHORS * PALM_CAM_HEAD_VALUES);
  palm_cam_tensor_stats("score[2016]", fout + PALM_CAM_SCORE_OFFSET / 4,
                        PALM_CAM_ANCHORS);

  /* Save the artifacts while the buffer still holds the frame that produced
   * this output - the sweep below overwrites it (r52 stored the grey fill and
   * mislabelled it as the input that generated palm_out.bin).
   */

  /* Decode before the uniform-fill sweep below, so the printed box and
   * keypoints belong to the CAMERA frame - r57 decoded at the end and
   * therefore always showed the last (white) fill result, which is why every
   * invocation printed the same numbers.
   */

  palm_cam_decode(fout);

  palm_cam_save(input, status.input_size, output, status.output_size, fout);

  /* r53: three runs with uniform black, grey and white inputs.  A network that
   * consumes the frame cannot return the same bytes for all of them; an equal
   * checksum says the input never entered the pipeline, and the driver's
   * engine sweep then shows which engine was supposed to fetch it.
   */

  for (i = 0; i < 3; i++)
    {
      static const uint8_t fill[3] = { 0x00, 0x80, 0xff };
      uint32_t fck = 0;

      memset(input, fill[i], status.input_size);
      up_clean_dcache((uintptr_t)input, (uintptr_t)input + status.input_size);

      memset(&params, 0, sizeof(params));
      params.input       = input;
      params.output      = output;
      params.input_size  = status.input_size;
      params.output_size = status.output_size;

      ret = ioctl(aiefd, AIE_CMD_FEED_INPUT, (unsigned long)&params);
      if (ret < 0)
        {
          palm_cam_out("palm_cam: fill 0x%02x run failed: %d\n",
                       (unsigned)fill[i], errno);
          continue;
        }

      up_invalidate_dcache((uintptr_t)output,
                           (uintptr_t)output + status.output_size);
      palm_cam_checksum(output, status.output_size, &fck, &minv, &maxv);
      {
        FAR const float *sc = (FAR const float *)
                              ((uintptr_t)output + PALM_CAM_SCORE_OFFSET);
        float pb = -1.0e30f;
        char pc[24];
        int a;

        for (a = 0; a < PALM_CAM_ANCHORS; a++)
          {
            if (sc[a] > pb)
              {
                pb = sc[a];
              }
          }

        palm_cam_fixed(pc, sizeof(pc), 1.0f / (1.0f + expf(-pb)));
        palm_cam_out("palm_cam: fill 0x%02x -> checksum 0x%08lx (%s), best "
                     "prob %s\n",
                     (unsigned)fill[i], (unsigned long)fck,
                     fck == ck ? "SAME as the camera frame - input ignored"
                               : "differs - the input reaches the network",
                     pc);
      }
      ck2 = fck;
    }

  palm_cam_out("palm_cam: done\n");

  close(fbfd);
  ioctl(vfd, VIDIOC_STREAMOFF, &type);
  close(vfd);
  close(aiefd);
  return 0;
}
