/****************************************************************************
 * apps/examples/pose_cam/pose_cam_main.c
 *
 * End-to-end pose estimation on the ATON NPU with the camera as the input
 * source:
 *
 *   IMX335 -> CSI-2 -> DCMIPP (RGB565 800x480, written straight into the LTDC
 *   framebuffer) -> CPU downscale to RGB888 256x256 -> ATON NPU (ST 994,
 *   yolov8n-pose) -> the 17 COCO keypoints of the best detection.
 *
 * Why this exists next to aie_probe: aie_probe proves the NPU runs the
 * official model deterministically.  This one proves it computes something
 * *meaningful* from a real image - point the camera at a person and the
 * printed keypoints should describe that person, cover the lens and every
 * score should collapse.
 *
 * Tensor formats come from the generated model, not from guessing:
 *
 *   input  (Transpose_7_out_0)  uint8 RGB888, 1x256x256x3 = 196608 B
 *   output (Transpose_663_out_0) float32, 1x56x1344, CHPos_First = 301056 B
 *
 * so the output is channel-major - out[channel * 1344 + anchor] - with
 * channel 0..3 the box, channel 4 the class score and 5..55 the 17 (x y conf)
 * keypoint triples, all in input-pixel units.  The transposed read is printed
 * once as a cross-check so a wrong assumption cannot pass unnoticed.
 *
 * Console output goes through the ATON driver's polled raw channel: the
 * interrupt-driven console in this tree can lose its TX wakeup, and the first
 * camera run froze in the middle of a line because of exactly that.
 *
 * If the TF card is mounted, the captured frame (PPM), the exact input tensor
 * and the raw output tensor are written to the mount point as well, so the
 * same data can be checked on a PC.
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

#include <nuttx/arch.h>                 /* up_clean/invalidate_dcache */
#include <nuttx/cache.h>
#include <nuttx/aie/ai_engine.h>
#include <nuttx/aie/stm32n6_aton_aie.h>
#include <nuttx/video/fb.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Tensors in the non-secure window, same placement aie_probe uses. */

#define POSE_CAM_INPUT_ADDR     0x24340000
#define POSE_CAM_OUTPUT_ADDR    0x24370000

#define POSE_CAM_NN_W           256
#define POSE_CAM_NN_H           256
#define POSE_CAM_NN_C           3
#define POSE_CAM_IN_LEN         (POSE_CAM_NN_W * POSE_CAM_NN_H * POSE_CAM_NN_C)

#define POSE_CAM_ANCHORS        1344
#define POSE_CAM_CHANNELS       56
#define POSE_CAM_KPTS           17
#define POSE_CAM_OUT_LEN        (POSE_CAM_ANCHORS * POSE_CAM_CHANNELS * 4)

#define POSE_CAM_FRAME_W        800
#define POSE_CAM_FRAME_H        480

/* ST uses 0.75 in app_config.h for this network */

#define POSE_CAM_CONF_THRESH    0.75f

#ifndef CONFIG_EXAMPLES_POSE_CAM_MOUNTPOINT
#  define CONFIG_EXAMPLES_POSE_CAM_MOUNTPOINT "/mnt/sdcard"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* Raw output channel: set once /dev/aie0 is open, so every line after that
 * survives a stalled console.
 */

static int g_pose_cam_fd = -1;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void pose_cam_out(FAR const char *fmt, ...)
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

  if (g_pose_cam_fd >= 0)
    {
      (void)ioctl(g_pose_cam_fd, STM32N6_ATON_CMD_PUTS, (unsigned long)buf);
    }
  else
    {
      fputs(buf, stdout);
      fflush(stdout);
    }
}

/* Print a float without pulling floating point into printf.  The tree is
 * built with CONFIG_LIBC_FLOATINGPOINT unset on purpose, so every float in
 * this file is rendered as fixed point by hand.
 */

static void pose_cam_fixed(FAR char *buf, size_t n, float v)
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

/* RGB565 800x480 (LTDC framebuffer) -> RGB888 256x256 model input.
 * Box average: the source block is 3..4 x 1..2 pixels for these geometries,
 * which is cheap and avoids point-sampling artefacts that ST's ISP does not
 * have to deal with.
 */

static void pose_cam_frame_to_input(FAR const uint16_t *fb, uint32_t stride,
                                    FAR uint8_t *dst)
{
  uint32_t stride_px = stride / 2;
  int x;
  int y;

  for (y = 0; y < POSE_CAM_NN_H; y++)
    {
      uint32_t sy0 = (uint32_t)y * POSE_CAM_FRAME_H / POSE_CAM_NN_H;
      uint32_t sy1 = (uint32_t)(y + 1) * POSE_CAM_FRAME_H / POSE_CAM_NN_H;
      uint32_t sy;

      if (sy1 <= sy0)
        {
          sy1 = sy0 + 1;
        }

      for (x = 0; x < POSE_CAM_NN_W; x++)
        {
          uint32_t sx0 = (uint32_t)x * POSE_CAM_FRAME_W / POSE_CAM_NN_W;
          uint32_t sx1 = (uint32_t)(x + 1) * POSE_CAM_FRAME_W / POSE_CAM_NN_W;
          uint32_t rsum = 0;
          uint32_t gsum = 0;
          uint32_t bsum = 0;
          uint32_t n = 0;
          uint32_t sx;

          if (sx1 <= sx0)
            {
              sx1 = sx0 + 1;
            }

          for (sy = sy0; sy < sy1 && sy < POSE_CAM_FRAME_H; sy++)
            {
              FAR const uint16_t *row = fb + sy * stride_px;

              for (sx = sx0; sx < sx1 && sx < POSE_CAM_FRAME_W; sx++)
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

/* The output is channel-major (chpos=CHPos_First): out[ch * 1344 + anchor].
 * The transposed accessor exists only for the cross-check print.
 */

static float pose_cam_val(FAR const float *out, int layout, int ch, int anchor)
{
  if (layout == 1)
    {
      return out[(size_t)ch * POSE_CAM_ANCHORS + anchor];
    }

  return out[(size_t)anchor * POSE_CAM_CHANNELS + ch];
}

static int pose_cam_scan(FAR const float *out, int layout,
                         FAR float *score, FAR float *box, FAR float *kpts)
{
  int best = -1;
  float best_score = -1.0e30f;
  int ch;
  int a;

  for (a = 0; a < POSE_CAM_ANCHORS; a++)
    {
      float s = pose_cam_val(out, layout, 4, a);

      if (s > best_score)
        {
          best_score = s;
          best = a;
        }
    }

  if (best < 0)
    {
      return -1;
    }

  *score = best_score;

  for (ch = 0; ch < 4; ch++)
    {
      box[ch] = pose_cam_val(out, layout, ch, best);
    }

  for (ch = 0; ch < POSE_CAM_KPTS * 3; ch++)
    {
      kpts[ch] = pose_cam_val(out, layout, 5 + ch, best);
    }

  return best;
}

/* Is this a real detection?  The layout is known from the model descriptor,
 * so this is a detection verdict, not a layout guess: a person-less frame
 * must come out as "nothing found" instead of being reported as a person at
 * score 0 with a zero-sized box.
 */

static bool pose_cam_detected(float score, FAR const float *box)
{
  if (score < 0.05f)
    {
      return false;
    }

  if (box[2] <= 2.0f || box[3] <= 2.0f ||
      box[2] > 512.0f || box[3] > 512.0f)
    {
      return false;
    }

  if (box[0] < -64.0f || box[0] > 320.0f ||
      box[1] < -64.0f || box[1] > 320.0f)
    {
      return false;
    }

  return true;
}

static void pose_cam_checksum(FAR const uint8_t *buf, size_t len,
                              FAR uint32_t *ck, FAR uint32_t *sum,
                              FAR uint8_t *minv, FAR uint8_t *maxv)
{
  uint32_t c = 0;
  uint32_t s = 0;
  uint8_t lo = 0xff;
  uint8_t hi = 0x00;
  size_t i;

  for (i = 0; i < len; i++)
    {
      uint8_t v = buf[i];

      c = (c << 1) ^ v;
      s += v;
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
  *sum = s;
  *minv = lo;
  *maxv = hi;
}

/* Frame + tensors to the card, so the result can be checked on a PC.  Errors
 * are reported and ignored: the console output is the primary result.
 */

static void pose_cam_save(FAR const uint8_t *input, size_t in_len,
                          FAR const uint8_t *output, size_t out_len,
                          FAR const uint8_t *rgb,
                          FAR const float *box, FAR const float *kpts,
                          float score, int anchor)
{
  FAR const char *mnt = CONFIG_EXAMPLES_POSE_CAM_MOUNTPOINT;
  char path[128];
  char a[24];
  char b[24];
  char c[24];
  FILE *fp;
  int k;

  snprintf(path, sizeof(path), "%s/pose_in.bin", mnt);
  fp = fopen(path, "wb");
  if (fp != NULL)
    {
      fwrite(input, 1, in_len, fp);
      fclose(fp);
      pose_cam_out("pose_cam: saved %s (%zu bytes)\n", path, in_len);
    }
  else
    {
      pose_cam_out("pose_cam: cannot write %s: %d (mount the card?)\n",
                   path, errno);
      return;
    }

  snprintf(path, sizeof(path), "%s/pose_out.bin", mnt);
  fp = fopen(path, "wb");
  if (fp != NULL)
    {
      fwrite(output, 1, out_len, fp);
      fclose(fp);
      pose_cam_out("pose_cam: saved %s (%zu bytes)\n", path, out_len);
    }

  /* Binary PPM of exactly what the network saw. */

  snprintf(path, sizeof(path), "%s/pose_frame.ppm", mnt);
  fp = fopen(path, "wb");
  if (fp != NULL)
    {
      fprintf(fp, "P6\n%d %d\n255\n", POSE_CAM_NN_W, POSE_CAM_NN_H);
      fwrite(rgb, 1, (size_t)POSE_CAM_NN_W * POSE_CAM_NN_H * 3, fp);
      fclose(fp);
      pose_cam_out("pose_cam: saved %s\n", path);
    }

  /* Keypoints as text, already decoded - the PC only has to draw them. */

  snprintf(path, sizeof(path), "%s/pose_kpts.txt", mnt);
  fp = fopen(path, "wb");
  if (fp != NULL)
    {
      pose_cam_fixed(a, sizeof(a), score);
      fprintf(fp, "# score %s anchor %d layout channel-major\n", a, anchor);

      pose_cam_fixed(a, sizeof(a), box[0]);
      pose_cam_fixed(b, sizeof(b), box[1]);
      pose_cam_fixed(c, sizeof(c), box[2]);
      fprintf(fp, "# box %s %s ", a, b);
      pose_cam_fixed(a, sizeof(a), box[3]);
      fprintf(fp, "%s\n", a);
      fprintf(fp, "# %d 256 256\n", POSE_CAM_KPTS);

      for (k = 0; k < POSE_CAM_KPTS; k++)
        {
          pose_cam_fixed(a, sizeof(a), kpts[k * 3 + 0]);
          pose_cam_fixed(b, sizeof(b), kpts[k * 3 + 1]);
          pose_cam_fixed(c, sizeof(c), kpts[k * 3 + 2]);
          fprintf(fp, "%02d %s %s %s\n", k, a, b, c);
        }

      fclose(fp);
      pose_cam_out("pose_cam: saved %s\n", path);
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
  FAR uint8_t *input  = (FAR uint8_t *)POSE_CAM_INPUT_ADDR;
  FAR uint8_t *output = (FAR uint8_t *)POSE_CAM_OUTPUT_ADDR;
  FAR const float *fout = (FAR const float *)POSE_CAM_OUTPUT_ADDR;
  FAR uint16_t *fb;
  float box[4];
  float kpts[POSE_CAM_KPTS * 3];
  float score = 0.0f;
  float cross;
  float alt_box[4];
  float alt_kpts[POSE_CAM_KPTS * 3];
  uint32_t ck = 0;
  uint32_t sum = 0;
  uint32_t ck2 = 0;
  uint8_t minv;
  uint8_t maxv;
  int runs = 1;
  int vfd = -1;
  int fbfd = -1;
  int aiefd = -1;
  int anchor = -1;
  int luma = 0;
  int ret;
  int i;
  int k;

  if (argc > 1)
    {
      runs = atoi(argv[1]);
      if (runs < 1)
        {
          runs = 1;
        }
    }

  /* 1. NPU session first: its handle also carries the debug output channel,
   *    so every message after this point reaches the console even when the
   *    console driver itself is stuck.
   */

  aiefd = open("/dev/aie0", O_RDWR);
  if (aiefd < 0)
    {
      fprintf(stderr, "pose_cam: open /dev/aie0 failed: %d\n", errno);
      return 1;
    }

  g_pose_cam_fd = aiefd;

  pose_cam_out("pose_cam: camera -> ATON NPU (ST 994 yolov8n-pose)\n");

  ret = ioctl(aiefd, AIE_CMD_LOAD, 0);
  if (ret < 0)
    {
      pose_cam_out("pose_cam: AIE_CMD_LOAD failed: %d\n", errno);
      close(aiefd);
      return 1;
    }

  memset(&status, 0, sizeof(status));
  ret = ioctl(aiefd, STM32N6_ATON_CMD_GET_STATUS, (unsigned long)&status);
  if (ret < 0)
    {
      pose_cam_out("pose_cam: GET_STATUS failed: %d\n", errno);
      close(aiefd);
      return 1;
    }

  pose_cam_out("pose_cam: model input %lu bytes, output %lu bytes\n",
               (unsigned long)status.input_size,
               (unsigned long)status.output_size);

  if (status.input_size != POSE_CAM_IN_LEN ||
      status.output_size != POSE_CAM_OUT_LEN)
    {
      pose_cam_out("pose_cam: unexpected tensor sizes for this test "
                   "(want %d/%d)\n", POSE_CAM_IN_LEN, POSE_CAM_OUT_LEN);
    }

  pose_cam_out("pose_cam: output tensor: float32, %dx%d, channel-major "
               "(chpos=First), channel 4 = score\n",
               POSE_CAM_CHANNELS, POSE_CAM_ANCHORS);

  memset(input, 0, status.input_size);

  /* 2. Camera: 800x480 RGB565 straight into the LTDC framebuffer, then the
   *    framebuffer is the frame source.
   */

  vfd = open("/dev/video0", O_RDONLY);
  if (vfd < 0)
    {
      pose_cam_out("pose_cam: open /dev/video0 failed: %d\n", errno);
      close(aiefd);
      return 1;
    }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = POSE_CAM_FRAME_W;
  fmt.fmt.pix.height      = POSE_CAM_FRAME_H;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;

  if (ioctl(vfd, VIDIOC_S_FMT, &fmt) < 0)
    {
      pose_cam_out("pose_cam: VIDIOC_S_FMT failed: %d\n", errno);
      close(vfd);
      close(aiefd);
      return 1;
    }

  if (ioctl(vfd, VIDIOC_STREAMON, &type) < 0)
    {
      pose_cam_out("pose_cam: VIDIOC_STREAMON failed: %d\n", errno);
      close(vfd);
      close(aiefd);
      return 1;
    }

  fbfd = open("/dev/fb0", O_RDONLY);
  if (fbfd < 0)
    {
      pose_cam_out("pose_cam: open /dev/fb0 failed: %d\n", errno);
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      close(vfd);
      close(aiefd);
      return 1;
    }

  if (ioctl(fbfd, FBIOGET_PLANEINFO, &pinfo) < 0)
    {
      pose_cam_out("pose_cam: FBIOGET_PLANEINFO failed: %d\n", errno);
      close(fbfd);
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      close(vfd);
      close(aiefd);
      return 1;
    }

  fb = (FAR uint16_t *)(uintptr_t)pinfo.fbmem;

  if (read(vfd, NULL, 0) < 0)
    {
      pose_cam_out("pose_cam: frame sync failed: %d\n", errno);
      close(fbfd);
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      close(vfd);
      close(aiefd);
      return 1;
    }

  pose_cam_frame_to_input(fb, pinfo.stride, input);

  for (i = 0; i < POSE_CAM_IN_LEN; i += 3)
    {
      luma += input[i];
    }

  luma = luma / (POSE_CAM_IN_LEN / 3);

  pose_cam_out("pose_cam: frame %dx%d -> input %dx%d, mean luma %d\n",
               POSE_CAM_FRAME_W, POSE_CAM_FRAME_H,
               POSE_CAM_NN_W, POSE_CAM_NN_H, luma);

  up_clean_dcache((uintptr_t)input, (uintptr_t)input + status.input_size);

  /* 3. Inference, twice on the same frame: the second checksum proves the
   *    run is reproducible, which is what makes a single capture meaningful.
   */

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
          pose_cam_out("pose_cam: AIE_CMD_FEED_INPUT failed: %d\n", errno);
          break;
        }

      up_invalidate_dcache((uintptr_t)output,
                           (uintptr_t)output + status.output_size);
      pose_cam_checksum(output, status.output_size, &ck, &sum, &minv, &maxv);

      pose_cam_out("pose_cam: run %d/%d ok: %lu us, %lu events, result %d, "
                   "output checksum 0x%08lx\n",
                   i + 1, runs, (unsigned long)params.usec,
                   (unsigned long)params.events, params.result,
                   (unsigned long)ck);

      if (i == 1 && ck != ck2)
        {
          pose_cam_out("pose_cam: WARNING different checksum on the same "
                       "frame (0x%08lx vs 0x%08lx)\n",
                       (unsigned long)ck, (unsigned long)ck2);
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

  pose_cam_out("pose_cam: output bytes min %u max %u\n",
               (unsigned)minv, (unsigned)maxv);

  /* 4. A few channels, so the descriptor's claim is visible in the data:
   *    channel 4 should look like scores, 0..3 like boxes, 5.. like
   *    keypoints.
   */

  {
    static const int chans[8] = { 0, 1, 2, 3, 4, 5, 53, 55 };
    char va[24];
    char vb[24];
    char vc[24];

    for (k = 0; k < 8; k++)
      {
        float lo = 1.0e30f;
        float hi = -1.0e30f;
        float avg = 0.0f;
        int ch = chans[k];

        for (i = 0; i < POSE_CAM_ANCHORS; i++)
          {
            float v = pose_cam_val(fout, 1, ch, i);

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

        avg /= (float)POSE_CAM_ANCHORS;
        pose_cam_fixed(va, sizeof(va), lo);
        pose_cam_fixed(vb, sizeof(vb), hi);
        pose_cam_fixed(vc, sizeof(vc), avg);
        pose_cam_out("pose_cam: ch[%02d] min %s max %s avg %s\n",
                     ch, va, vb, vc);
      }
  }

  /* 5. Decode: channel-major, channel 4 is the score.  The transposed read
   *    is printed once so the two can be compared by eye.
   */

  anchor = pose_cam_scan(fout, 1, &score, box, kpts);
  if (anchor < 0)
    {
      pose_cam_out("pose_cam: output scan failed\n");
      close(fbfd);
      ioctl(vfd, VIDIOC_STREAMOFF, &type);
      close(vfd);
      close(aiefd);
      return 1;
    }

  {
    float alt = 0.0f;
    int alt_anchor = pose_cam_scan(fout, 0, &alt, alt_box, alt_kpts);
    char sa[24];
    char sb[24];

    pose_cam_fixed(sa, sizeof(sa), alt);
    pose_cam_fixed(sb, sizeof(sb), score);
    pose_cam_out("pose_cam: transposed cross-check: best %s (anchor %d) vs "
                 "channel-major %s (anchor %d)\n",
                 sa, alt_anchor, sb, anchor);
    cross = alt;
  }

  /* 6. Artifacts for the PC before the long print, so they exist even if the
   *    console decides to misbehave again.
   */

  pose_cam_save(input, status.input_size, output, status.output_size,
                input, box, kpts, score, anchor);

  /* 7. Verdict and keypoints. */

  {
    char sa[24];
    char sb[24];
    char sc[24];
    int above = 0;

    for (i = 0; i < POSE_CAM_ANCHORS; i++)
      {
        if (pose_cam_val(fout, 1, 4, i) >= POSE_CAM_CONF_THRESH)
          {
            above++;
          }
      }

    pose_cam_fixed(sa, sizeof(sa), score);
    pose_cam_fixed(sb, sizeof(sb), box[0]);
    pose_cam_fixed(sc, sizeof(sc), box[1]);

    if (!pose_cam_detected(score, box))
      {
        pose_cam_out("pose_cam: no person detected (best score %s at anchor "
                     "%d, %d anchor(s) above 0.75) - aim the camera at a "
                     "person and run again\n", sa, anchor, above);
        (void)cross;
        close(fbfd);
        ioctl(vfd, VIDIOC_STREAMOFF, &type);
        close(vfd);
        close(aiefd);
        return 0;
      }

    pose_cam_out("pose_cam: person score %s at anchor %d, %d anchor(s) "
                 "above 0.75, box at %s,%s\n",
                 sa, anchor, above, sb, sc);
  }

  for (k = 0; k < POSE_CAM_KPTS; k++)
    {
      char sa[24];
      char sb[24];
      char sc[24];

      pose_cam_fixed(sa, sizeof(sa), kpts[k * 3 + 0]);
      pose_cam_fixed(sb, sizeof(sb), kpts[k * 3 + 1]);
      pose_cam_fixed(sc, sizeof(sc), kpts[k * 3 + 2]);
      pose_cam_out("pose_cam: kp[%02d] x=%s y=%s conf=%s\n", k, sa, sb, sc);
    }

  pose_cam_out("pose_cam: done\n");

  close(fbfd);
  ioctl(vfd, VIDIOC_STREAMOFF, &type);
  close(vfd);
  close(aiefd);
  return 0;
}
