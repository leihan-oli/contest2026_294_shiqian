/****************************************************************************
 * apps/examples/cam_save/cam_save_main.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to you under the Apache License, Version
 * 2.0 (the "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied.  See the License for the specific language governing
 * permissions and limitations under the License.
 *
 * Camera frame capture to TF card (dataset collection).
 *
 * Usage (NSH):
 *   cam_save [nsamples] [label] [w] [h] [gray|color] [x] [y] [rw] [rh]
 *
 *   nsamples : number of frames to capture (default 20)
 *   label    : class name used in the file name (default "sample")
 *   w h      : output size in pixels (default 320x240)
 *   gray     : 8bpp greyscale BMP (default); "color" selects 24bpp BGR
 *   x y rw rh: source rectangle inside the 800x480 frame (default all)
 *
 * Flow:
 *  1. open /dev/video0, VIDIOC_S_FMT 800x480 RGB565, STREAMON
 *  2. for each sample: read(fd, NULL, 0) frame-sync, then box-filter the
 *     selected rectangle of the LTDC framebuffer (via /dev/fb0) into the
 *     output image
 *  3. write a BMP file to /mnt/sdcard/ (FATFS, mounted by the board)
 *
 * The output files are named {label}_{seq:03d}.bmp, e.g. sample_001.bmp.
 * The card is removed afterwards and read on the host PC for training.
 *
 * Examples:
 *   cam_save 300 blink_open                 320x240 grey, whole frame
 *   cam_save 300 blink_open 640 480 color   640x480 colour, whole frame
 *   cam_save 300 eye 256 256 gray 160 60 480 360
 *                                           crop a 480x360 window first
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
#include <string.h>
#include <errno.h>

#include <nuttx/video/fb.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_EXAMPLES_CAM_SAVE_MOUNTPOINT
#  define CONFIG_EXAMPLES_CAM_SAVE_MOUNTPOINT "/mnt/sdcard"
#endif

/* Source geometry: DCMIPP streams the live view into the 800x480 LTDC
 * framebuffer, and that is what this tool reads back.
 */

#define FRAME_W           800
#define FRAME_H           480

/* Default output geometry.  The original build decimated the whole frame
 * 800x480 -> 96x96 with a point sampler (8.3x), which aliased badly and
 * left far too little eye detail for the blink/gaze models.  The default
 * is now a larger box-filtered image, and every value can be overridden
 * on the command line - including an explicit source rectangle, so a
 * future DCMIPP crop window can be evaluated without recompiling.
 */

#define DEF_OUT_W         320
#define DEF_OUT_H         240

/* BMP header 14 + DIB 40 + data */

#define BMP_HEADER_SIZE   54

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static void bmp_put_u16(FAR uint8_t *p, uint16_t v)
{
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
}

static void bmp_put_u32(FAR uint8_t *p, uint32_t v)
{
  p[0] = (uint8_t)(v & 0xff);
  p[1] = (uint8_t)((v >> 8) & 0xff);
  p[2] = (uint8_t)((v >> 16) & 0xff);
  p[3] = (uint8_t)((v >> 24) & 0xff);
}

/****************************************************************************
 * Name: bmp_write_header
 *
 * Description:
 *   Emit a 14-byte BITMAPFILEHEADER followed by a 40-byte BITMAPINFOHEADER
 *   for an uncompressed image of the given geometry.  An 8bpp image is
 *   followed by a 256-entry greyscale palette written by the caller.
 *
 ****************************************************************************/

static void bmp_write_header(FAR FILE *fp, int w, int h, int bpp,
                             uint32_t imgsz)
{
  FAR uint8_t header[BMP_HEADER_SIZE];
  uint32_t offset;

  offset = BMP_HEADER_SIZE + (bpp == 8 ? 1024 : 0);

  memset(header, 0, sizeof(header));
  header[0] = 'B';
  header[1] = 'M';
  bmp_put_u32(&header[2], offset + imgsz);        /* file size */
  bmp_put_u32(&header[10], offset);               /* pixel data offset */
  bmp_put_u32(&header[14], 40);                   /* DIB header size */
  bmp_put_u32(&header[18], (uint32_t)w);
  bmp_put_u32(&header[22], (uint32_t)h);
  bmp_put_u16(&header[26], 1);                    /* colour planes */
  bmp_put_u16(&header[28], (uint16_t)bpp);
  bmp_put_u32(&header[34], imgsz);
  bmp_put_u32(&header[38], 2835);                 /* 72 dpi */
  bmp_put_u32(&header[42], 2835);
  bmp_put_u32(&header[46], bpp == 8 ? 256 : 0);   /* palette entries */
  bmp_put_u32(&header[50], bpp == 8 ? 256 : 0);

  fwrite(header, 1, sizeof(header), fp);
}

/****************************************************************************
 * Name: bmp_write_palette
 *
 * Description:
 *   Emit the 256-entry greyscale palette that follows the headers of an
 *   8bpp BMP.  It is built from the loop index so no RAM table is needed.
 *
 ****************************************************************************/

static int bmp_write_palette(FAR FILE *fp)
{
  FAR uint8_t entry[4];
  int i;

  for (i = 0; i < 256; i++)
    {
      entry[0] = (uint8_t)i;        /* B */
      entry[1] = (uint8_t)i;        /* G */
      entry[2] = (uint8_t)i;        /* R */
      entry[3] = 0;                 /* reserved */

      if (fwrite(entry, 1, sizeof(entry), fp) != sizeof(entry))
        {
          return -EIO;
        }
    }

  return 0;
}

/****************************************************************************
 * Name: convert_row
 *
 * Description:
 *   Box-filter one output row straight out of the LTDC framebuffer into the
 *   caller supplied line buffer.
 *
 *   Writing the BMP row by row keeps the working set at a single line
 *   (a few KB at most).  Converting a whole frame first would need up to
 *   ~900 KB of contiguous heap for a 640x480 colour capture, which the
 *   default heap cannot reliably satisfy.
 *
 ****************************************************************************/

static void convert_row(FAR const uint16_t *fb, uint32_t stride,
                        int rx, int ry, int rw, int rh,
                        int out_w, int out_h, int y, int colour,
                        FAR uint8_t *row)
{
  uint32_t stride_px = stride / 2;
  int sy0 = ry + (y * rh) / out_h;
  int sy1 = ry + ((y + 1) * rh) / out_h;
  int x;

  if (sy1 <= sy0)
    {
      sy1 = sy0 + 1;
    }

  for (x = 0; x < out_w; x++)
    {
      int sx0 = rx + (x * rw) / out_w;
      int sx1 = rx + ((x + 1) * rw) / out_w;
      uint32_t sr = 0;
      uint32_t sg = 0;
      uint32_t sb = 0;
      uint32_t n = 0;
      int sx;
      int sy;

      if (sx1 <= sx0)
        {
          sx1 = sx0 + 1;
        }

      for (sy = sy0; sy < sy1; sy++)
        {
          FAR const uint16_t *srow = &fb[sy * stride_px];

          for (sx = sx0; sx < sx1; sx++)
            {
              uint16_t px = srow[sx];

              sr += (px >> 11) & 0x1f;
              sg += (px >> 5) & 0x3f;
              sb += px & 0x1f;
              n++;
            }
        }

      /* RGB565 block averages expanded to 8 bits per channel */

      sr = (sr * 255) / (n * 31);
      sg = (sg * 255) / (n * 63);
      sb = (sb * 255) / (n * 31);

      if (colour)
        {
          /* BMP pixel order is B, G, R */

          row[x * 3 + 0] = (uint8_t)sb;
          row[x * 3 + 1] = (uint8_t)sg;
          row[x * 3 + 2] = (uint8_t)sr;
        }
      else
        {
          row[x] = (uint8_t)((sr * 77 + sg * 150 + sb * 29) >> 8);
        }
    }
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct v4l2_format fmt;
  struct fb_planeinfo_s pinfo;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  FAR const char *label = "sample";
  FAR uint16_t *fb = NULL;
  FAR uint8_t *row = NULL;
  FILE *fp;
  int rowsz;
  uint32_t imgsz;
  char path[128];
  int nsamples = 20;
  int out_w = DEF_OUT_W;
  int out_h = DEF_OUT_H;
  int colour = 0;
  int rx = 0;
  int ry = 0;
  int rw = FRAME_W;
  int rh = FRAME_H;
  int fd;
  int fbfd;
  int i;

  /* cam_save [nsamples] [label] [w] [h] [gray|color] [x] [y] [rw] [rh] */

  if (argc > 1)
    {
      nsamples = atoi(argv[1]);
    }

  if (argc > 2)
    {
      label = argv[2];
    }

  if (argc > 3)
    {
      out_w = atoi(argv[3]);
    }

  if (argc > 4)
    {
      out_h = atoi(argv[4]);
    }

  if (argc > 5)
    {
      /* Accept color / colour / rgb */

      colour = (argv[5][0] == 'c' || argv[5][0] == 'C' ||
                argv[5][0] == 'r' || argv[5][0] == 'R');
    }

  if (argc > 9)
    {
      rx = atoi(argv[6]);
      ry = atoi(argv[7]);
      rw = atoi(argv[8]);
      rh = atoi(argv[9]);
    }

  if (out_w < 8 || out_h < 8 || out_w > 2 * FRAME_W || out_h > 2 * FRAME_H)
    {
      fprintf(stderr, "cam_save: bad output geometry %dx%d\n", out_w, out_h);
      return 1;
    }

  if (rx < 0 || ry < 0 || rw < 8 || rh < 8 ||
      rx + rw > FRAME_W || ry + rh > FRAME_H)
    {
      fprintf(stderr, "cam_save: bad source rect %d,%d %dx%d\n",
              rx, ry, rw, rh);
      return 1;
    }

  /* One BMP row is the entire working set: each frame is converted and
   * streamed to the card line by line, so a 640x480 colour capture no
   * longer needs a ~900 KB contiguous heap block.
   */

  rowsz = colour ? ((out_w * 3 + 3) & ~3) : ((out_w + 3) & ~3);
  imgsz = (uint32_t)rowsz * (uint32_t)out_h;

  row = malloc(rowsz);
  if (row == NULL)
    {
      fprintf(stderr, "cam_save: malloc failed\n");
      return 1;
    }

  /* 1. Open video device and configure format */

  fd = open("/dev/video0", O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "cam_save: open /dev/video0 failed: %d\n", errno);
      free(row);
      return 1;
    }

  memset(&fmt, 0, sizeof(fmt));
  fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = FRAME_W;
  fmt.fmt.pix.height      = FRAME_H;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
  if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
    {
      fprintf(stderr, "cam_save: VIDIOC_S_FMT failed: %d\n", errno);
      close(fd);
      free(row);
      return 1;
    }

  if (ioctl(fd, VIDIOC_STREAMON, &type) < 0)
    {
      fprintf(stderr, "cam_save: VIDIOC_STREAMON failed: %d\n", errno);
      close(fd);
      free(row);
      return 1;
    }

  /* 2. Open the framebuffer for direct pixel access */

  fbfd = open("/dev/fb0", O_RDONLY);
  if (fbfd < 0)
    {
      fprintf(stderr, "cam_save: open /dev/fb0 failed: %d\n", errno);
      ioctl(fd, VIDIOC_STREAMOFF, &type);
      close(fd);
      free(row);
      return 1;
    }

  if (ioctl(fbfd, FBIOGET_PLANEINFO, &pinfo) < 0)
    {
      fprintf(stderr, "cam_save: FBIOGET_PLANEINFO failed: %d\n", errno);
      close(fbfd);
      ioctl(fd, VIDIOC_STREAMOFF, &type);
      close(fd);
      free(row);
      return 1;
    }

  fb = (FAR uint16_t *)(uintptr_t)pinfo.fbmem;
  printf("cam_save: capturing %d frames as '%s' to %s\n",
         nsamples, label, CONFIG_EXAMPLES_CAM_SAVE_MOUNTPOINT);
  printf("cam_save: output %dx%d %s, source rect %d,%d %dx%d\n",
         out_w, out_h, colour ? "colour" : "gray", rx, ry, rw, rh);

  /* 3. Capture loop: frame-sync then downscale + save */

  for (i = 0; i < nsamples; i++)
    {
      int y;

      /* Frame-sync: read(fd, NULL, 0) waits for the next completed frame
       * (DCMIPP writes it into the LTDC framebuffer).
       */

      if (read(fd, NULL, 0) < 0)
        {
          fprintf(stderr, "cam_save: frame-sync failed: %d\n", errno);
          break;
        }

      snprintf(path, sizeof(path),
               "%s/%s_%03d.bmp",
               CONFIG_EXAMPLES_CAM_SAVE_MOUNTPOINT, label, i + 1);

      fp = fopen(path, "wb");
      if (fp == NULL)
        {
          fprintf(stderr, "cam_save: fopen %s failed: %d\n", path, errno);
          break;
        }

      bmp_write_header(fp, out_w, out_h, colour ? 24 : 8, imgsz);

      if (!colour && bmp_write_palette(fp) < 0)
        {
          fclose(fp);
          break;
        }

      /* BMP rows are stored bottom-up; convert one row at a time */

      for (y = out_h - 1; y >= 0; y--)
        {
          convert_row(fb, pinfo.stride, rx, ry, rw, rh,
                      out_w, out_h, y, colour, row);

          if (fwrite(row, 1, rowsz, fp) != (size_t)rowsz)
            {
              break;
            }
        }

      fclose(fp);

      printf("cam_save: saved %s\n", path);
    }

  /* 4. Cleanup */

  ioctl(fd, VIDIOC_STREAMOFF, &type);
  close(fbfd);
  close(fd);
  free(row);
  printf("cam_save: done (%d/%d)\n", i, nsamples);
  return 0;
}
