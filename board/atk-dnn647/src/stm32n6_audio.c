/****************************************************************************
 * vendor/openvela/boards/atk-dnn647/src/stm32n6_audio.c
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
 * ATK-DNN647 audio output: ES8388 codec on SAI1.
 *
 * The codec sits on SAI1 and takes both its bit clock and its MCLK from this
 * side, so the board owns the whole path: pins, codec registers, the WAV
 * file, and the loop that feeds the transmit block.
 *
 * The register values are the vendor's, not derived.  ES8388 configuration
 * is a long sequence of undocumented-by-formula choices, and the vendor BSP
 * for this exact board is known to produce sound; transcribing it removes a
 * whole category of guesswork.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>

#include <nuttx/arch.h>
#include <syslog.h>

#include "arm_internal.h"
#include "stm32n6_gpio.h"
#include "stm32n6_sai.h"
#include "stm32n6_audio.h"

#if defined(CONFIG_STM32_SAI1)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The codec's control bus is plain GPIO, driven in software.  Open drain so
 * the codec can pull SDA low for its acknowledge without either side driving
 * against the other, and slow because the vendor's timing is a 2 us delay
 * per half bit (about 250 kHz).
 */

#define GPIO_ES_SCL \
  (GPIO_MODE_OUTPUT | GPIO_OTYPE_OD | GPIO_SPEED_LOW | \
   GPIO_PUPD_PU | GPIO_PORTE | GPIO_PIN(13))

#define GPIO_ES_SDA \
  (GPIO_MODE_OUTPUT | GPIO_OTYPE_OD | GPIO_SPEED_LOW | \
   GPIO_PUPD_PU | GPIO_PORTE | GPIO_PIN(14))

#define ES8388_I2C_ADDR     0x10    /* fixed, per the codec's data sheet */

#define ES_AUDIO_CHUNK      4096    /* bytes per write to the transmit block */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct wav_info_s
{
  uint32_t rate;          /* samples per second */
  uint16_t channels;      /* 1 or 2 */
  uint16_t bits;          /* bits per sample */
  uint32_t data_off;      /* byte offset of the samples */
  uint32_t data_len;      /* byte length of the samples */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_audio_ready;

/****************************************************************************
 * Private Functions: software I2C
 ****************************************************************************/

static void es_delay(void)
{
  up_udelay(2);
}

static void es_scl(bool value)
{
  stm32n6_gpiowrite(GPIO_ES_SCL, value);
}

static void es_sda(bool value)
{
  stm32n6_gpiowrite(GPIO_ES_SDA, value);
}

static bool es_sda_read(void)
{
  return stm32n6_gpioread(GPIO_ES_SDA);
}

static void es_start(void)
{
  es_sda(true);
  es_scl(true);
  es_delay();

  es_sda(false);        /* SDA falls while SCL is high: start condition */
  es_delay();
  es_scl(false);
  es_delay();
}

static void es_stop(void)
{
  es_sda(false);
  es_delay();

  es_scl(true);
  es_delay();

  es_sda(true);         /* SDA rises while SCL is high: stop condition */
  es_delay();
}

static bool es_write_byte(uint8_t byte)
{
  bool ack;
  int  i;

  for (i = 0; i < 8; i++)
    {
      es_sda((byte & 0x80) != 0);
      es_delay();

      es_scl(true);
      es_delay();
      es_scl(false);
      es_delay();

      byte <<= 1;
    }

  /* Ninth clock: release SDA and let the codec answer.  A pulled-down line
   * is the acknowledge.
   */

  es_sda(true);
  es_delay();
  es_scl(true);
  es_delay();

  ack = !es_sda_read();

  es_scl(false);
  es_delay();

  return ack;
}

static int es_write_reg(uint8_t reg, uint8_t value)
{
  es_start();

  if (!es_write_byte(ES8388_I2C_ADDR << 1) ||
      !es_write_byte(reg) ||
      !es_write_byte(value))
    {
      es_stop();
      return -EIO;
    }

  es_stop();
  return 0;
}

/****************************************************************************
 * Private Functions: codec bring-up
 ****************************************************************************/

/****************************************************************************
 * Name: es8388_configure
 *
 * Description:
 *   The vendor's power-up sequence.  Register numbers and values are theirs.
 *
 *   Registers 0x0D and 0x18 set the ADC and DAC oversampling, and 0x08 says
 *   MCLK is not divided: together they fix the ratio between MCLK and the
 *   sample rate, which is why the clock tree on the transmit side has to
 *   produce what it does.
 *
 ****************************************************************************/

static int es8388_configure(void)
{
  static const struct
  {
    uint8_t reg;
    uint8_t value;
  } seq[] =
  {
    { 0x00, 0x80 },   /* soft reset */
    { 0x00, 0x00 },
    { 0x01, 0x58 },   /* power management */
    { 0x01, 0x50 },
    { 0x02, 0xF3 },
    { 0x02, 0xF0 },
    { 0x03, 0x09 },   /* microphone bias off */
    { 0x00, 0x06 },   /* reference and 500k driver on */
    { 0x04, 0x00 },   /* DAC power: no channel on yet */
    { 0x08, 0x00 },   /* MCLK not divided */
    { 0x2B, 0x80 },   /* DACLRC follows ADCLRC */
    { 0x09, 0x88 },   /* ADC PGA +24 dB */
    { 0x0C, 0x4C },   /* ADC data: left to left, 16 bit */
    { 0x0D, 0x02 },   /* ADC oversampling */
    { 0x10, 0x00 },   /* ADC digital volume: no attenuation */
    { 0x11, 0x00 },
    { 0x17, 0x18 },   /* DAC 16 bit */
    { 0x18, 0x02 },   /* DAC oversampling */
    { 0x1A, 0x00 },   /* DAC digital volume: no attenuation */
    { 0x1B, 0x00 },
    { 0x27, 0xB8 },   /* left mixer */
    { 0x2A, 0xB8 },   /* right mixer */
  };

  unsigned i;
  int      ret;

  stm32n6_configgpio(GPIO_ES_SCL);
  stm32n6_configgpio(GPIO_ES_SDA);

  es_scl(true);
  es_sda(true);

  for (i = 0; i < sizeof(seq) / sizeof(seq[0]); i++)
    {
      ret = es_write_reg(seq[i].reg, seq[i].value);
      if (ret < 0)
        {
          syslog(LOG_ERR, "audio: codec write %02x failed\n", seq[i].reg);
          return ret;
        }

      /* The reset pair needs settling time before anything else lands. */

      if (seq[i].reg == 0x00 && seq[i].value == 0x00)
        {
          up_mdelay(100);
        }
    }

  /* Then the five settings the vendor's player makes after init, in the same
   * order and with the same encodings.  Note that 0x02 is overwritten here:
   * the sequence above ends with both converters off, and these bits are
   * active low.
   *
   *   0x02  adda_cfg(1, 0)     DAC on, ADC off
   *   0x04  output_cfg(1, 1)   both DAC output channels on (3 << 4 | 3 << 2)
   *   0x2E  headphone volume
   *   0x30  speaker volume, 0-33
   *   0x17  Philips I2S, 16 bit  ((fmt << 1) | (len << 3), fmt 0, len 3)
   */

  es_write_reg(0x02, 0x0A);
  es_write_reg(0x04, 0x3C);
  es_write_reg(0x2E, 0x0A);
  es_write_reg(0x2F, 0x0A);
  es_write_reg(0x30, 0x0F);
  es_write_reg(0x31, 0x0F);
  es_write_reg(0x17, 0x18);

  up_mdelay(100);
  return 0;
}

/****************************************************************************
 * Private Functions: WAV
 ****************************************************************************/

static uint32_t wav_u32(FAR const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
         ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t wav_u16(FAR const uint8_t *p)
{
  return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

/****************************************************************************
 * Name: wav_parse
 *
 * Description:
 *   Read the fmt and data chunks.
 *
 *   A WAV file is a RIFF container of chunks and the interesting ones are not
 *   always the first two, so this walks them rather than assuming offsets.
 *   Only the fields playback actually needs are pulled out.
 *
 ****************************************************************************/

static int wav_parse(FAR FILE *fp, FAR struct wav_info_s *info)
{
  uint8_t hdr[12];
  size_t  off = 12;

  memset(info, 0, sizeof(*info));

  if (fread(hdr, 1, sizeof(hdr), fp) != sizeof(hdr) ||
      memcmp(hdr, "RIFF", 4) != 0 || memcmp(hdr + 8, "WAVE", 4) != 0)
    {
      return -EINVAL;
    }

  for (;;)
    {
      uint8_t chunk[8];
      uint32_t size;

      if (fread(chunk, 1, sizeof(chunk), fp) != sizeof(chunk))
        {
          return -EINVAL;
        }

      size = wav_u32(chunk + 4);

      if (memcmp(chunk, "fmt ", 4) == 0)
        {
          uint8_t fmt[16];

          if (size < sizeof(fmt) || fread(fmt, 1, sizeof(fmt), fp) != sizeof(fmt))
            {
              return -EINVAL;
            }

          info->channels = wav_u16(fmt + 2);
          info->rate     = wav_u32(fmt + 4);
          info->bits     = wav_u16(fmt + 14);

          /* Skip whatever of the chunk was not read. */

          if (size > sizeof(fmt))
            {
              fseek(fp, (long)(size - sizeof(fmt)), SEEK_CUR);
            }
        }
      else if (memcmp(chunk, "data", 4) == 0)
        {
          info->data_off = (uint32_t)off + 8;
          info->data_len = size;
          break;
        }
      else
        {
          /* Chunks are word aligned; an odd length carries a pad byte. */

          fseek(fp, (long)(size + (size & 1)), SEEK_CUR);
        }

      off += 8 + size + (size & 1);
    }

  if (info->rate == 0 || info->channels == 0 || info->bits == 0)
    {
      return -EINVAL;
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int stm32n6_audio_init(void)
{
  int ret;

  if (g_audio_ready)
    {
      return 0;
    }

  /* Codec first, transmit block second: the block starts MCLK as soon as it
   * is enabled, and the codec should already know what to do with it.
   */

  ret = es8388_configure();
  if (ret < 0)
    {
      return ret;
    }

  ret = stm32n6_sai_output_init(44100, 2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "audio: transmit path init failed: %d\n", ret);
      return ret;
    }

  g_audio_ready = true;
  syslog(LOG_INFO, "audio: ES8388 on SAI1 ready, 44.1 kHz stereo\n");
  return 0;
}

void stm32n6_audio_set_volume(uint8_t volume)
{
  if (volume > 33)
    {
      volume = 33;
    }

  es_start();
  es_write_byte(ES8388_I2C_ADDR << 1);
  es_write_byte(0x30);
  es_write_byte(volume);
  es_stop();

  es_start();
  es_write_byte(ES8388_I2C_ADDR << 1);
  es_write_byte(0x31);
  es_write_byte(volume);
  es_stop();
}

int stm32n6_audio_play_wav(FAR const char *path)
{
  FAR uint8_t       *buf;
  FAR FILE          *fp;
  struct wav_info_s  info;
  uint32_t           left;
  uint64_t           started;
  uint32_t           elapsed_ms;
  uint32_t           expected_ms;
  int                ret = 0;

  if (path == NULL)
    {
      return -EINVAL;
    }

  ret = stm32n6_audio_init();
  if (ret < 0)
    {
      return ret;
    }

  fp = fopen(path, "rb");
  if (fp == NULL)
    {
      syslog(LOG_ERR, "audio: cannot open %s: %d\n", path, errno);
      return -errno;
    }

  ret = wav_parse(fp, &info);
  if (ret < 0)
    {
      syslog(LOG_ERR, "audio: %s is not a WAV this can play\n", path);
      fclose(fp);
      return ret;
    }

  syslog(LOG_INFO, "audio: %s  %lu Hz, %u ch, %u bit, %lu bytes\n",
         path, (unsigned long)info.rate, info.channels, info.bits,
         (unsigned long)info.data_len);

  /* The transmit path is wired for one rate and one layout.  Saying so beats
   * playing it at the wrong speed, which sounds like a driver bug.
   */

  if (info.rate != 44100 || info.channels != 2 || info.bits != 16)
    {
      syslog(LOG_ERR, "audio: only 44.1 kHz 16-bit stereo is wired up\n");
      fclose(fp);
      return -EINVAL;
    }

  buf = malloc(ES_AUDIO_CHUNK);
  if (buf == NULL)
    {
      fclose(fp);
      return -ENOMEM;
    }

  fseek(fp, (long)info.data_off, SEEK_SET);

  started = (uint64_t)clock_systime_ticks() * 1000 /
            (uint64_t)CLK_TCK;

  left = info.data_len;
  while (left > 0)
    {
      size_t want = left > ES_AUDIO_CHUNK ? ES_AUDIO_CHUNK : left;
      size_t got  = fread(buf, 1, want, fp);

      if (got < 2)
        {
          break;
        }

      /* The block takes samples, not bytes, and wants an even count. */

      got &= ~(size_t)1;

      ret = stm32n6_sai_output_write((FAR const int16_t *)buf,
                                     (uint32_t)(got / 2));
      if (ret < 0)
        {
          syslog(LOG_ERR, "audio: transmit failed: %d\n", ret);
          break;
        }

      left -= got;
    }

  free(buf);
  fclose(fp);

  /* Report how long it actually took against how long the file says it
   * should.  This is the only check on the clock chain that does not need a
   * scope: a wrong PLL2 divider shows up as a proportional error here.
   */

  elapsed_ms = (uint32_t)((uint64_t)clock_systime_ticks() * 1000 /
                          (uint64_t)CLK_TCK) - (uint32_t)started;

  expected_ms = (uint32_t)((uint64_t)info.data_len * 1000 /
                           ((uint64_t)info.rate * info.channels *
                            (info.bits / 8)));

  syslog(LOG_INFO, "audio: played %lu ms of audio in %lu ms "
         "(%lu.%02lu%% of nominal)\n",
         (unsigned long)expected_ms, (unsigned long)elapsed_ms,
         (unsigned long)(elapsed_ms * 100 /
                         (expected_ms ? expected_ms : 1)),
         (unsigned long)(elapsed_ms * 10000 /
                         (expected_ms ? expected_ms : 1) % 100));

  return ret;
}

#endif /* CONFIG_STM32_SAI1 */
