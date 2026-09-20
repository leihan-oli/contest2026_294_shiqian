/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_video.c
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
 * STM32N6 V4L2 camera capture driver (/dev/video0).
 *
 * Wraps the DCMIPP + IMX335 drivers behind the NuttX V4L2 character-device
 * framework (video_register + struct v4l2_ops_s).  The DCMIPP PIPE1
 * demosaics the IMX335 RAW10 CSI-2 stream into RGB565 800x480 directly
 * into the shared LTDC framebuffer - an extra 768KB capture buffer is not
 * affordable in SRAM - so this driver exposes:
 *
 *   - VIDIOC_STREAMON : power the IMX335, configure DCMIPP/CSI and start
 *     continuous PIPE1 capture into /dev/fb0 (live camera view on LCD),
 *   - read() / VIDIOC_DQBUF : block until a frame completes, then copy
 *     the 800x480 RGB565 frame to the caller,
 *   - VIDIOC_STREAMOFF : stop capture.
 *
 * Frame completion is delivered from the DCMIPP ISR via
 * stm32n6_dcmipp_set_frame_callback().
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <sys/videoio.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <debug.h>
#include <poll.h>

#include <nuttx/fs/fs.h>
#include <nuttx/mutex.h>
#include <nuttx/semaphore.h>
#include <nuttx/video/video.h>

#include "arm_internal.h"
#include "nvic.h"
#include "stm32n6_dcmipp.h"
#include "stm32n6_imx335.h"

#ifdef CONFIG_STM32N6_VIDEO

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define STM32N6_VIDEO_WIDTH    800
#define STM32N6_VIDEO_HEIGHT   480
#define STM32N6_VIDEO_BPP      16
#define STM32N6_VIDEO_STRIDE   (STM32N6_VIDEO_WIDTH * \
                                 (STM32N6_VIDEO_BPP >> 3))
#define STM32N6_VIDEO_FRMSIZE  (STM32N6_VIDEO_STRIDE * STM32N6_VIDEO_HEIGHT)

/* The framebuffer accessor lives in board_bringup.c (CONFIG_VIDEO_FB). */

extern uint32_t stm32n6_board_get_framebuffer(void);

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct stm32n6_video_dev_s
{
  mutex_t                lock;         /* Serialises start/stop/read */
  sem_t                  frame_sem;    /* Posted from ISR on each frame */
  FAR struct pollfd     *pollfds;      /* Registered poll waiter */
  volatile bool          frame_ready;  /* A frame has completed */
  volatile bool          streaming;    /* Capture active */
  bool                   initialized;  /* One-shot init done */
  bool                   pipeline_up;  /* IMX335 powered + configured and
                                        * DCMIPP initialised: only the
                                        * capture request is re-armed */
  uint32_t               seq;          /* Frame sequence counter */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct stm32n6_video_dev_s g_stm32n6_video;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32n6_video_start(void);
static int stm32n6_video_stop(void);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_video_frame_cb
 *
 * Description:
 *   DCMIPP PIPE1 frame-complete callback (invoked from the DCMIPP ISR).
 *   Marks a frame ready, wakes a blocking read()/DQBUF and notifies poll.
 *
 ****************************************************************************/

static void stm32n6_video_frame_cb(uint32_t pipe)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;

  priv->frame_ready = true;
  nxsem_post(&priv->frame_sem);

  if (priv->pollfds != NULL)
    {
      poll_notify(&priv->pollfds, 1, POLLIN);
    }
}

/****************************************************************************
 * Name: stm32n6_video_stage
 *
 * Description:
 *   Report how long the step just finished took, and restart the clock for
 *   the next one.  Only useful because the whole start sequence runs once per
 *   frame: the application stops the stream to freeze the buffer while it
 *   infers, and starts it again afterwards.
 *
 ****************************************************************************/

static void stm32n6_video_stage(FAR const char *name, FAR clock_t *t0)
{
  clock_t now = clock_systime_ticks();

  _info("stm32n6_video: %-18s %lu ms\n", name,
        (unsigned long)TICK2MSEC(now - *t0));
  *t0 = now;
}

/****************************************************************************
 * Name: stm32n6_video_start
 *
 * Description:
 *   Full capture start sequence (mirrors cam_main.c cam_start()):
 *     IMX335 power-up -> configure -> DCMIPP init -> sensor streaming
 *     -> VC0 active -> continuous PIPE1 capture into the framebuffer.
 *
 ****************************************************************************/

/* Capture destination.  Zero means "whatever the board hands out as the
 * panel framebuffer", which is what the camera mode wants: it draws its
 * overlay straight into the buffer the LTDC scans out, so there is nothing to
 * copy.  The eye-UI mode points it at the HyperRAM instead, because there the
 * panel belongs to LVGL and a capture landing in that buffer would wipe the
 * grid on every frame.
 */

static uint32_t g_capture_dest;

uint32_t stm32n6_video_capture_dest(void)
{
  return g_capture_dest != 0 ? g_capture_dest :
                               stm32n6_board_get_framebuffer();
}

void stm32n6_video_set_capture_dest(uint32_t addr)
{
  g_capture_dest = addr;
}

static int stm32n6_video_start(void)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;
  uint32_t fbaddr;
  clock_t t0;
  bool cold;
  int ret;

  if (priv->streaming)
    {
      return OK;
    }

  t0 = clock_systime_ticks();

  fbaddr = stm32n6_video_capture_dest();
  if (fbaddr == 0)
    {
      verr("stm32n6_video: no framebuffer (CONFIG_VIDEO_FB?)\n");
      return -ENODEV;
    }

  /* The bring-up below is one-shot.
   *
   * stm32n6_video_stop() only clears PIPE1's capture request and waits for
   * P1CPTACT: it does not power the sensor down, does not stop CSI VC0 and
   * leaves the DCMIPP configuration untouched.  The sensor is therefore
   * still streaming while the application runs its inference, so all
   * STREAMON has to do for the next frame is re-arm the capture.
   *
   * Re-running the whole sequence (power-on, sensor configuration, DCMIPP
   * init, stream start) cost 2.6 s per frame for no gain: it re-ran the
   * IMX335 reset/standby delays and re-programmed registers that already
   * held exactly those values.  Nothing outside this driver requires the
   * cold path, and it is needed only once per power cycle.
   */

  cold = !priv->pipeline_up;

  if (cold)
    {
      ret = stm32n6_imx335_power_on();
      if (ret < 0)
        {
          verr("stm32n6_video: IMX335 power-on failed: %d\n", -ret);
          return ret;
        }

      stm32n6_video_stage("imx335 power-on", &t0);

      ret = stm32n6_imx335_configure();
      if (ret < 0)
        {
          verr("stm32n6_video: IMX335 configure failed: %d\n", -ret);
          return ret;
        }

      stm32n6_video_stage("imx335 configure", &t0);

      ret = stm32n6_dcmipp_init();
      if (ret < 0)
        {
          verr("stm32n6_video: DCMIPP init failed: %d\n", -ret);
          return ret;
        }

      stm32n6_video_stage("dcmipp init", &t0);

      ret = stm32n6_imx335_start_stream();
      if (ret < 0)
        {
          verr("stm32n6_video: IMX335 start stream failed: %d\n", -ret);
          return ret;
        }

      stm32n6_video_stage("imx335 start_stream", &t0);

      priv->pipeline_up = true;
    }

  /* Report which path was taken, and the D-Cache state with it: the cache
   * being off is invisible in most logs but costs 5-10x on every memory
   * access, so name it next to the timing it explains.
   *
   * The DCMIPP counters come along because the warm path deliberately
   * leaves CSI VC0 running while the pipe is not asked to capture.  If the
   * receiver ever wedged under that, the CSI error flags would latch and
   * the frame count would stop: both are visible here, per frame, next to
   * the start that caused them.
   */

  _info("stm32n6_video: %s start (NVIC_CFGCON=0x%08lx, "
        "dcmipp frames=%lu overruns=%lu err=0x%08lx)\n",
        cold ? "cold" : "warm",
        (unsigned long)getreg32(NVIC_CFGCON),
        (unsigned long)stm32n6_dcmipp_get_frame_count(),
        (unsigned long)stm32n6_dcmipp_get_overrun_count(),
        (unsigned long)stm32n6_dcmipp_get_error_code());

  /* Arm the frame-complete callback before starting capture. */

  stm32n6_dcmipp_set_frame_callback(stm32n6_video_frame_cb);

  /* start_capture() re-programs the PIPE1 ISP chain, clears the stale CSI
   * flags and re-arms VC0 + PIPE1.  It is the only step that has to run
   * for every captured frame.
   */

  ret = stm32n6_dcmipp_start_capture(fbaddr);
  if (ret < 0)
    {
      verr("stm32n6_video: DCMIPP start failed: %d\n", -ret);
      stm32n6_dcmipp_set_frame_callback(NULL);
      return ret;
    }

  stm32n6_video_stage("dcmipp start_capture", &t0);

  priv->streaming = true;
  priv->seq = 0;
  vinfo("stm32n6_video: capture started -> 0x%08lx\n",
        (unsigned long)fbaddr);

  return OK;
}

/****************************************************************************
 * Name: stm32n6_video_stop
 *
 * Description:
 *   Stop PIPE1 capture, detach the frame callback and reset the ready
 *   state so the next STREAMON starts clean.
 *
 ****************************************************************************/

static int stm32n6_video_stop(void)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;
  int ret;

  if (!priv->streaming)
    {
      return OK;
    }

  ret = stm32n6_dcmipp_stop();
  stm32n6_dcmipp_set_frame_callback(NULL);

  priv->streaming = false;
  priv->frame_ready = false;

  /* Drain any pending frame completions. */

  while (nxsem_trywait(&priv->frame_sem) == 0)
    {
    }

  return ret;
}

/****************************************************************************
 * v4l2_ops: device capability
 ****************************************************************************/

static int stm32n6_video_querycap(FAR struct file *filep,
                                  FAR struct v4l2_capability *cap)
{
  memset(cap, 0, sizeof(*cap));
  strlcpy((FAR char *)cap->driver, "stm32n6-dcmipp",
          sizeof(cap->driver));
  strlcpy((FAR char *)cap->card, "IMX335 CSI-2 RGB565",
          sizeof(cap->card));
  strlcpy((FAR char *)cap->bus_info, "platform:stm32n6",
          sizeof(cap->bus_info));
  cap->version        = 0x0100;
  cap->capabilities   = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_READWRITE |
                        V4L2_CAP_STREAMING;
  cap->device_caps    = cap->capabilities;
  return OK;
}

/****************************************************************************
 * v4l2_ops: format enumeration / negotiation
 ****************************************************************************/

static int stm32n6_video_enum_fmt(FAR struct file *filep,
                                  FAR struct v4l2_fmtdesc *f)
{
  if (f->index != 0 || f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
      return -EINVAL;
    }

  memset(f, 0, sizeof(*f));
  f->index       = 0;
  f->type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  f->pixelformat = V4L2_PIX_FMT_RGB565;
  strlcpy((FAR char *)f->description, "RGB565 800x480",
          sizeof(f->description));
  return OK;
}

static void stm32n6_video_fill_pix(FAR struct v4l2_pix_format *pix)
{
  memset(pix, 0, sizeof(*pix));
  pix->width        = STM32N6_VIDEO_WIDTH;
  pix->height       = STM32N6_VIDEO_HEIGHT;
  pix->pixelformat  = V4L2_PIX_FMT_RGB565;
  pix->field        = V4L2_FIELD_NONE;
  pix->bytesperline = STM32N6_VIDEO_STRIDE;
  pix->sizeimage    = STM32N6_VIDEO_FRMSIZE;
  pix->colorspace   = V4L2_COLORSPACE_SRGB;
}

static int stm32n6_video_g_fmt(FAR struct file *filep,
                               FAR struct v4l2_format *fmt)
{
  if (fmt->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
      return -EINVAL;
    }

  stm32n6_video_fill_pix(&fmt->fmt.pix);
  return OK;
}

static int stm32n6_video_try_fmt(FAR struct file *filep,
                                 FAR struct v4l2_format *fmt)
{
  if (fmt->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
      return -EINVAL;
    }

  /* The DCMIPP pipe is configured for a single resolution; only accept
   * RGB565 800x480 and report the actual (only) supported format back. */

  stm32n6_video_fill_pix(&fmt->fmt.pix);
  return OK;
}

static int stm32n6_video_s_fmt(FAR struct file *filep,
                               FAR struct v4l2_format *fmt)
{
  return stm32n6_video_try_fmt(filep, fmt);
}

/****************************************************************************
 * v4l2_ops: streaming control
 ****************************************************************************/

static int stm32n6_video_streamon(FAR struct file *filep,
                                  FAR enum v4l2_buf_type *type)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;
  int ret;

  if (*type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
      return -EINVAL;
    }

  nxmutex_lock(&priv->lock);
  ret = stm32n6_video_start();
  nxmutex_unlock(&priv->lock);
  return ret;
}

static int stm32n6_video_streamoff(FAR struct file *filep,
                                   FAR enum v4l2_buf_type *type)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;
  int ret;

  if (*type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
      return -EINVAL;
    }

  nxmutex_lock(&priv->lock);
  ret = stm32n6_video_stop();
  nxmutex_unlock(&priv->lock);
  return ret;
}

/****************************************************************************
 * v4l2_ops: streaming parameters (frame rate)
 ****************************************************************************/

static int stm32n6_video_g_parm(FAR struct file *filep,
                                FAR struct v4l2_streamparm *parm)
{
  if (parm->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
      return -EINVAL;
    }

  parm->parm.capture.capability     = V4L2_CAP_TIMEPERFRAME;
  parm->parm.capture.timeperframe.numerator   = 1;
  parm->parm.capture.timeperframe.denominator = 30;
  return OK;
}

static int stm32n6_video_s_parm(FAR struct file *filep,
                                FAR struct v4l2_streamparm *parm)
{
  return stm32n6_video_g_parm(filep, parm);
}

/****************************************************************************
 * v4l2_ops: buffer management (single-buffer mode, backing store is the
 * shared LTDC framebuffer)
 ****************************************************************************/

static int stm32n6_video_reqbufs(FAR struct file *filep,
                                 FAR struct v4l2_requestbuffers *reqbufs)
{
  if (reqbufs->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
      return -EINVAL;
    }

  /* Single hardware capture buffer (the framebuffer); clamp to 1. */

  reqbufs->count = 1;
  return OK;
}

static int stm32n6_video_querybuf(FAR struct file *filep,
                                  FAR struct v4l2_buffer *buf)
{
  if (buf->type != V4L2_BUF_TYPE_VIDEO_CAPTURE || buf->index != 0)
    {
      return -EINVAL;
    }

  buf->length = STM32N6_VIDEO_FRMSIZE;
  buf->memory = V4L2_MEMORY_MMAP;
  buf->m.offset = 0;
  return OK;
}

static int stm32n6_video_qbuf(FAR struct file *filep,
                              FAR struct v4l2_buffer *buf)
{
  if (buf->type != V4L2_BUF_TYPE_VIDEO_CAPTURE || buf->index != 0)
    {
      return -EINVAL;
    }

  /* Single buffer is always queued; nothing to do. */

  return OK;
}

static int stm32n6_video_dqbuf(FAR struct file *filep,
                               FAR struct v4l2_buffer *buf)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;
  int ret;

  if (buf->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
    {
      return -EINVAL;
    }

  nxmutex_lock(&priv->lock);
  if (!priv->streaming)
    {
      ret = stm32n6_video_start();
      if (ret < 0)
        {
          nxmutex_unlock(&priv->lock);
          return ret;
        }
    }

  nxmutex_unlock(&priv->lock);

  /* Block until a frame completes. */

  ret = nxsem_wait(&priv->frame_sem);
  if (ret < 0)
    {
      return ret;   /* Interrupted by a signal */
    }

  priv->frame_ready = false;
  priv->seq++;

  memset(buf, 0, sizeof(*buf));
  buf->index     = 0;
  buf->type      = V4L2_BUF_TYPE_VIDEO_CAPTURE;
  buf->bytesused = STM32N6_VIDEO_FRMSIZE;
  buf->memory    = V4L2_MEMORY_MMAP;
  buf->m.offset  = 0;
  buf->length    = STM32N6_VIDEO_FRMSIZE;
  buf->sequence  = priv->seq;
  return OK;
}

/****************************************************************************
 * v4l2_ops: dispatch table
 ****************************************************************************/

static const struct v4l2_ops_s g_stm32n6_video_ops =
{
  .querycap   = stm32n6_video_querycap,
  .reqbufs    = stm32n6_video_reqbufs,
  .querybuf   = stm32n6_video_querybuf,
  .qbuf       = stm32n6_video_qbuf,
  .dqbuf      = stm32n6_video_dqbuf,
  .g_fmt      = stm32n6_video_g_fmt,
  .s_fmt      = stm32n6_video_s_fmt,
  .try_fmt    = stm32n6_video_try_fmt,
  .enum_fmt   = stm32n6_video_enum_fmt,
  .g_parm     = stm32n6_video_g_parm,
  .s_parm     = stm32n6_video_s_parm,
  .streamon   = stm32n6_video_streamon,
  .streamoff  = stm32n6_video_streamoff,
};

/****************************************************************************
 * Character-driver methods
 ****************************************************************************/

static int stm32n6_video_open(FAR struct file *filep)
{
  return OK;
}

static int stm32n6_video_close(FAR struct file *filep)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;

  if (priv->streaming)
    {
      stm32n6_video_stop();
    }

  return OK;
}

static ssize_t stm32n6_video_read(FAR struct file *filep,
                                  FAR char *buffer, size_t buflen)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;
  uint32_t fbaddr;
  int ret;

  /* Two modes:
   *   read(fd, buf, FRMSIZE) -> wait for a frame and copy it to buf
   *   read(fd, NULL, 0)      -> frame-sync only: wait until a complete
   *                             frame landed in the shared framebuffer
   *                             (zero-copy; the caller reads /dev/fb0
   *                             directly - needed because a 768KB user
   *                             buffer cannot be allocated in SRAM).
   */

  if (buffer == NULL && buflen != 0)
    {
      return -EINVAL;
    }

  if (buflen != 0 && buflen < STM32N6_VIDEO_FRMSIZE)
    {
      return -EINVAL;
    }

  nxmutex_lock(&priv->lock);
  if (!priv->streaming)
    {
      ret = stm32n6_video_start();
      if (ret < 0)
        {
          nxmutex_unlock(&priv->lock);
          return ret;
        }
    }

  nxmutex_unlock(&priv->lock);

  /* Wait for a complete frame (posted from the DCMIPP ISR). */

  ret = nxsem_wait(&priv->frame_sem);
  if (ret < 0)
    {
      return ret;   /* Interrupted by a signal */
    }

  priv->frame_ready = false;
  priv->seq++;

  if (buflen == 0)
    {
      return 0;   /* Frame-sync only: frame is in the framebuffer. */
    }

  /* Copy the latest frame from the shared framebuffer to the caller. */

  fbaddr = stm32n6_video_capture_dest();
  memcpy(buffer, (FAR const void *)fbaddr, STM32N6_VIDEO_FRMSIZE);

  return STM32N6_VIDEO_FRMSIZE;
}

static int stm32n6_video_poll(FAR struct file *filep,
                              FAR struct pollfd *fds, bool setup)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;

  if (setup)
    {
      priv->pollfds = fds;

      if (priv->frame_ready)
        {
          fds->revents |= POLLIN;
        }
    }
  else
    {
      priv->pollfds = NULL;
    }

  return OK;
}

static const struct file_operations g_stm32n6_video_fops =
{
  .open  = stm32n6_video_open,
  .close = stm32n6_video_close,
  .read  = stm32n6_video_read,
  .poll  = stm32n6_video_poll,
};

static const struct v4l2_s g_stm32n6_v4l2 =
{
  .vops = &g_stm32n6_video_ops,
  .fops = &g_stm32n6_video_fops,
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32n6_video_initialize
 *
 * Description:
 *   Register the V4L2 camera capture device (/dev/video0).
 *
 ****************************************************************************/

int stm32n6_video_initialize(void)
{
  FAR struct stm32n6_video_dev_s *priv = &g_stm32n6_video;
  int ret;

  if (priv->initialized)
    {
      return OK;
    }

  nxmutex_init(&priv->lock);
  nxsem_init(&priv->frame_sem, 0, 0);
  priv->initialized = true;

  ret = video_register(CONFIG_STM32N6_VIDEO_DEVPATH, &g_stm32n6_v4l2);
  if (ret < 0)
    {
      verr("stm32n6_video: video_register failed: %d\n", ret);
      return ret;
    }

  vinfo("stm32n6_video: registered %s\n",
        CONFIG_STM32N6_VIDEO_DEVPATH);

  return OK;
}

#endif /* CONFIG_STM32N6_VIDEO */
