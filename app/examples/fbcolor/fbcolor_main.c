/****************************************************************************
 * apps/examples/fbcolor/fbcolor_main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <nuttx/video/fb.h>

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/* Convert an RGB888 triple to RGB565. */

static uint16_t rgb888_to_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
  return (uint16_t)(((r & 0xf8) << 8) | ((g & 0xfc) << 3) | (b >> 3));
}

/* Parse a color name into RGB565.  Returns false if unknown. */

static bool parse_color_name(const char *name, uint16_t *color)
{
  struct color_name_s
  {
    const char *name;
    uint16_t    rgb565;
  };

  static const struct color_name_s g_colors[] =
  {
    { "black",   0x0000 },
    { "white",   0xffff },
    { "red",     0xf800 },
    { "green",   0x07e0 },
    { "blue",    0x001f },
    { "yellow",  0xffe0 },
    { "cyan",    0x07ff },
    { "magenta", 0xf81f },
    { "orange",  0xfd20 },
    { "purple",  0x8010 },
    { "gray",    0x8410 },
    { "grey",    0x8410 },
  };

  int i;

  for (i = 0; i < (int)(sizeof(g_colors) / sizeof(g_colors[0])); i++)
    {
      if (strcasecmp(name, g_colors[i].name) == 0)
        {
          *color = g_colors[i].rgb565;
          return true;
        }
    }

  return false;
}

/****************************************************************************
 * Private Functions
 ****************************************************************************/



/****************************************************************************
 * Public Functions
 ****************************************************************************/

int main(int argc, FAR char *argv[])
{
  struct fb_videoinfo_s vinfo;
  struct fb_planeinfo_s pinfo;
  FAR uint16_t *fb;
  uint16_t color;
  size_t npixels;
  size_t i;
  int fd;
  int ret;

  /* Parse the color argument:
   *   fbcolor <name>         - e.g. red, green, 0xf800
   *   fbcolor <r> <g> <b>    - RGB888 (0..255 each)
   */

  if (argc == 2)
    {
      if (!parse_color_name(argv[1], &color))
        {
          color = (uint16_t)strtoul(argv[1], NULL, 0);
        }
    }
  else if (argc == 4)
    {
      color = rgb888_to_rgb565((uint8_t)strtoul(argv[1], NULL, 0),
                               (uint8_t)strtoul(argv[2], NULL, 0),
                               (uint8_t)strtoul(argv[3], NULL, 0));
    }
  else
    {
      fprintf(stderr,
              "Usage: fbcolor <name|0xRRRR>   e.g. fbcolor red, fbcolor 0xf800\n"
              "       fbcolor <r> <g> <b>     e.g. fbcolor 255 0 0\n"
              "Names: black white red green blue yellow cyan magenta\n"
              "       orange purple gray\n");
      return -1;
    }

  /* Open the framebuffer device */

  fd = open("/dev/fb0", O_RDWR);
  if (fd < 0)
    {
      fprintf(stderr, "ERROR: open /dev/fb0 failed: %d\n", errno);
      return -1;
    }

  /* Query the video / plane geometry */

  ret = ioctl(fd, FBIOGET_VIDEOINFO, (unsigned long)&vinfo);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: FBIOGET_VIDEOINFO failed: %d\n", errno);
      close(fd);
      return -1;
    }

  ret = ioctl(fd, FBIOGET_PLANEINFO, (unsigned long)&pinfo);
  if (ret < 0)
    {
      fprintf(stderr, "ERROR: FBIOGET_PLANEINFO failed: %d\n", errno);
      close(fd);
      return -1;
    }

  /* Map the framebuffer into the caller's address space */

  fb = (FAR uint16_t *)mmap(NULL, pinfo.fblen, PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, 0);
  if (fb == MAP_FAILED)
    {
      fprintf(stderr, "ERROR: mmap failed: %d\n", errno);
      close(fd);
      return -1;
    }

  /* Fill the whole framebuffer with the requested color.  The panel is
   * RGB565 (2 bytes/pixel) for this LTDC configuration.
   */

  npixels = pinfo.fblen / 2;
  for (i = 0; i < npixels; i++)
    {
      fb[i] = color;
    }

  /* Flush the D-Cache so the LTDC DMA reads the freshly written pixels.
   * CPU writes may sit in cache lines; without this the LTDC reads stale
   * physical memory and the panel never updates.  up_flush_dcache() is
   * implemented unconditionally in arch/arm/src/armv8-m/arm_cache.c even
   * though cache.h guards its prototype behind CONFIG_ARCH_DCACHE. */

  /* Flush the D-Cache so the LTDC DMA reads the freshly written pixels. */

  extern void up_flush_dcache(uintptr_t start, uintptr_t end);
  up_flush_dcache((uintptr_t)fb, (uintptr_t)fb + pinfo.fblen);

  printf("Filled %lux%lu with 0x%04x\n",
         (unsigned long)vinfo.xres, (unsigned long)vinfo.yres, color);

  munmap(fb, pinfo.fblen);
  close(fd);
  return 0;
}
