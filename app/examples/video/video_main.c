/****************************************************************************
 * apps/examples/video/video_main.c
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
 * V4L2 camera capture test for the STM32N647 ATK-DNN647 board.
 *
 * Usage (NSH):  video [nframes]
 *
 *  1. open /dev/video0
 *  2. VIDIOC_QUERYCAP
 *  3. VIDIOC_S_FMT  -> 800x480 RGB565
 *  4. VIDIOC_STREAMON (DCMIPP captures continuously into /dev/fb0)
 *  5. read(fd, NULL, 0) x N -> frame-sync (zero-copy); the DCMIPP writes
 *     each frame into the LTDC framebuffer, so the frame pixels are read
 *     straight from /dev/fb0 (no 768KB user buffer is affordable).
 *  6. VIDIOC_STREAMOFF, close.
 *
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/videoio.h>
#include <sys/ioctl.h>

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <nuttx/video/fb.h>

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct v4l2_capability cap;
  struct v4l2_format fmt;
  enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  struct fb_planeinfo_s pinfo;
  FAR uint16_t *fb = NULL;
  int fd;
  int fbfd;
  int nframes = 5;
  int i;

  if (argc > 1)
    {
      nframes = atoi(argv[1]);
    }

  /* 1. Open the V4L2 device */

  fd = open("/dev/video0", O_RDONLY);
  if (fd < 0)
    {
      fprintf(stderr, "video: open /dev/video0 failed: %d\n", errno);
      return 1;
    }

  /* 2. Query the device capability */

  memset(&cap, 0, sizeof(cap));
  if (ioctl(fd, VIDIOC_QUERYCAP, (unsigned long)&cap) < 0)
    {
      fprintf(stderr, "video: VIDIOC_QUERYCAP failed: %d\n", errno);
      close(fd);
      return 1;
    }

  printf("video: driver=%s card=%s bus=%s caps=0x%08x\n",
         cap.driver, cap.card, cap.bus_info, cap.capabilities);

  /* 3. Set the capture format (800x480 RGB565) */

  memset(&fmt, 0, sizeof(fmt));
  fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  fmt.fmt.pix.width       = 800;
  fmt.fmt.pix.height      = 480;
  fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
  if (ioctl(fd, VIDIOC_S_FMT, (unsigned long)&fmt) < 0)
    {
      fprintf(stderr, "video: VIDIOC_S_FMT failed: %d\n", errno);
      close(fd);
      return 1;
    }

  printf("video: fmt %ux%u bpl=%u size=%u\n",
         fmt.fmt.pix.width, fmt.fmt.pix.height,
         fmt.fmt.pix.bytesperline, fmt.fmt.pix.sizeimage);

  /* Locate the shared framebuffer (the DCMIPP writes frames into it).
   * In a zero-copy flow the caller reads pixels from here after each
   * frame-sync read(). */

  fbfd = open("/dev/fb0", O_RDONLY);
  if (fbfd < 0)
    {
      fprintf(stderr, "video: open /dev/fb0 failed: %d\n", errno);
      close(fd);
      return 1;
    }

  memset(&pinfo, 0, sizeof(pinfo));
  if (ioctl(fbfd, FBIOGET_PLANEINFO, (unsigned long)&pinfo) < 0)
    {
      fprintf(stderr, "video: FBIOGET_PLANEINFO failed: %d\n", errno);
      close(fbfd);
      close(fd);
      return 1;
    }

  fb = (FAR uint16_t *)pinfo.fbmem;
  printf("video: framebuffer @ 0x%08lx (bpp=%u stride=%u)\n",
         (unsigned long)(uintptr_t)fb, pinfo.bpp, pinfo.stride);

  /* 4. Start the stream (DCMIPP + IMX335 capture into the framebuffer) */

  if (ioctl(fd, VIDIOC_STREAMON, (unsigned long)&type) < 0)
    {
      fprintf(stderr, "video: VIDIOC_STREAMON failed: %d\n", errno);
      close(fbfd);
      close(fd);
      return 1;
    }

  /* 5. Frame-sync read: read(fd, NULL, 0) blocks until a complete frame
   *    has landed in the framebuffer; pixels are read directly from fb. */

  for (i = 0; i < nframes; i++)
    {
      ssize_t n = read(fd, NULL, 0);
      if (n < 0)
        {
          fprintf(stderr, "video: frame-sync read failed: %d\n", errno);
          break;
        }

      printf("video: frame %d  first px = 0x%04x 0x%04x 0x%04x 0x%04x\n",
             i, fb[0], fb[1], fb[2], fb[3]);
    }

  /* 6. Stop the stream and close */

  ioctl(fd, VIDIOC_STREAMOFF, (unsigned long)&type);
  close(fbfd);
  close(fd);

  printf("video: done\n");
  return 0;
}
