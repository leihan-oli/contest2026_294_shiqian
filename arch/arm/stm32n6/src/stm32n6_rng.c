/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_rng.c
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
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include <errno.h>

#include <nuttx/irq.h>
#include <nuttx/arch.h>
#include <nuttx/mutex.h>
#include <nuttx/fs/fs.h>
#include <nuttx/drivers/drivers.h>

#include "hardware/stm32_rng.h"
#include "hardware/stm32_rcc.h"
#include "arm_internal.h"

#if defined(CONFIG_STM32_RNG)
#if defined(CONFIG_DEV_RANDOM) || defined(CONFIG_DEV_URANDOM_ARCH)

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Timeout for CONDRST completion in milliseconds */

#define RNG_CONDRST_TIMEOUT_MS  100

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int stm32_rng_initialize(void);
static int stm32_rng_init_hw(void);
static int stm32_rng_interrupt(int irq, void *context, void *arg);
static void stm32_rng_enable(void);
static void stm32_rng_disable(void);
static ssize_t stm32_rng_read(struct file *filep, char *buffer,
                               size_t buflen);

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct rng_dev_s
{
  mutex_t rd_devlock;   /* Threads can only exclusively access the RNG */
  sem_t rd_readsem;     /* To block until the buffer is filled */
  char *rd_buf;         /* Pointer to read buffer */
  size_t rd_buflen;     /* Remaining bytes to fill */
  uint32_t rd_lastval;  /* Last generated value (FIPS check) */
  bool rd_first;        /* True if next value is first after enable */
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct rng_dev_s g_rngdev =
{
  .rd_devlock = NXMUTEX_INITIALIZER,
  .rd_readsem = SEM_INITIALIZER(0),
};

static const struct file_operations g_rngops =
{
  NULL,            /* open */
  NULL,            /* close */
  stm32_rng_read,  /* read */
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_rng_init_hw
 *
 * Description:
 *   Initialize the RNG hardware: enable RCC clock and perform CONDRST
 *   initialization sequence required by STM32N6.
 *
 * Input Parameters:
 *   None
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno on failure.
 *
 ****************************************************************************/

static int stm32_rng_init_hw(void)
{
  uint32_t tickstart;
  uint32_t regval;

  /* Enable RNG peripheral clock */

  modifyreg32(STM32_RCC_AHB3ENR, 0, RCC_AHB3ENR_RNGEN);

  /* Small delay for clock to stabilize */

  up_mdelay(1);

  /* Disable RNG before configuration */

  regval = getreg32(STM32_RNG_CR);
  regval &= ~RNG_CR_RNGEN;
  putreg32(regval, STM32_RNG_CR);

  /* CONDRST initialization sequence (STM32N6 specific):
   * 1. Set CONDRST=1 with config bits (CED=0 to enable clock detection)
   * 2. Clear CONDRST=0
   * 3. Wait until hardware clears CONDRST (indicates config applied)
   */

  regval = getreg32(STM32_RNG_CR);
  regval |= RNG_CR_CONDRST;
  regval &= ~RNG_CR_CED;  /* Enable clock error detection */
  putreg32(regval, STM32_RNG_CR);

  /* Clear CONDRST to trigger the reset sequence */

  regval &= ~RNG_CR_CONDRST;
  putreg32(regval, STM32_RNG_CR);

  /* Wait for CONDRST to be cleared by hardware */

  tickstart = clock_systime_ticks();
  while (getreg32(STM32_RNG_CR) & RNG_CR_CONDRST)
    {
      if ((clock_systime_ticks() - tickstart) >
          MSEC2TICK(RNG_CONDRST_TIMEOUT_MS))
        {
          if (getreg32(STM32_RNG_CR) & RNG_CR_CONDRST)
            {
              _err("ERROR: CONDRST timeout\n");
              return -ETIMEDOUT;
            }
        }
    }

  return OK;
}

/****************************************************************************
 * Name: stm32_rng_initialize
 *
 * Description:
 *   Attach the RNG interrupt handler.
 *
 ****************************************************************************/

static int stm32_rng_initialize(void)
{
  int ret;

  ret = irq_attach(STM32_IRQ_RNG, stm32_rng_interrupt, NULL);
  if (ret < 0)
    {
      _err("ERROR: Could not attach IRQ %d: %d\n",
             STM32_IRQ_RNG, ret);
      return ret;
    }

  return OK;
}

/****************************************************************************
 * Name: stm32_rng_enable
 *
 * Description:
 *   Enable the RNG with interrupt generation.
 *
 ****************************************************************************/

static void stm32_rng_enable(void)
{
  uint32_t regval;

  g_rngdev.rd_first = true;

  /* Enable RNG and interrupts */

  regval = getreg32(STM32_RNG_CR);
  regval |= RNG_CR_RNGEN;
  regval |= RNG_CR_IE;
  putreg32(regval, STM32_RNG_CR);

  up_enable_irq(STM32_IRQ_RNG);
}

/****************************************************************************
 * Name: stm32_rng_disable
 *
 * Description:
 *   Disable the RNG and its interrupt.
 *
 ****************************************************************************/

static void stm32_rng_disable(void)
{
  uint32_t regval;

  up_disable_irq(STM32_IRQ_RNG);

  regval = getreg32(STM32_RNG_CR);
  regval &= ~RNG_CR_IE;
  regval &= ~RNG_CR_RNGEN;
  putreg32(regval, STM32_RNG_CR);
}

/****************************************************************************
 * Name: stm32_rng_interrupt
 *
 * Description:
 *   RNG interrupt handler. Reads random data from the DR register,
 *   performs FIPS continuous random number generator test, and fills
 *   the read buffer.
 *
 ****************************************************************************/

static int stm32_rng_interrupt(int irq, void *context, void *arg)
{
  uint32_t sr;
  uint32_t data;

  sr = getreg32(STM32_RNG_SR);

  /* Check for clock error interrupt status */

  if (sr & RNG_SR_CEIS)
    {
      /* Clear it and try again */

      putreg32(sr & ~RNG_SR_CEIS, STM32_RNG_SR);
      return OK;
    }

  /* Check for seed error interrupt status */

  if (sr & RNG_SR_SEIS)
    {
      uint32_t cr;

      /* Clear seed error, then disable/enable the RNG to recover */

      putreg32(sr & ~RNG_SR_SEIS, STM32_RNG_SR);
      cr = getreg32(STM32_RNG_CR);
      cr &= ~RNG_CR_RNGEN;
      putreg32(cr, STM32_RNG_CR);
      cr |= RNG_CR_RNGEN;
      putreg32(cr, STM32_RNG_CR);
      return OK;
    }

  /* Data ready must be set */

  if (!(sr & RNG_SR_DRDY))
    {
      return OK;
    }

  data = getreg32(STM32_RNG_DR);

  /* FIPS PUB 140-2 continuous test: first value after enable is saved
   * for comparison. Subsequent values must differ from the previous one.
   */

  if (g_rngdev.rd_first)
    {
      g_rngdev.rd_first = false;
      g_rngdev.rd_lastval = data;
      return OK;
    }

  if (g_rngdev.rd_lastval == data)
    {
      /* Two consecutive identical values — discard and retry */

      return OK;
    }

  g_rngdev.rd_lastval = data;

  /* Fill the read buffer */

  if (g_rngdev.rd_buflen >= 4)
    {
      g_rngdev.rd_buflen -= 4;
      *(uint32_t *)&g_rngdev.rd_buf[g_rngdev.rd_buflen] = data;
    }
  else
    {
      while (g_rngdev.rd_buflen > 0)
        {
          g_rngdev.rd_buf[--g_rngdev.rd_buflen] = (char)data;
          data >>= 8;
        }
    }

  if (g_rngdev.rd_buflen == 0)
    {
      /* Buffer filled, stop further interrupts */

      stm32_rng_disable();
      nxsem_post(&g_rngdev.rd_readsem);
    }

  return OK;
}

/****************************************************************************
 * Name: stm32_rng_read
 *
 * Description:
 *   Read random data from the RNG. This is a blocking read that fills
 *   the caller's buffer with hardware-generated random bytes.
 *
 ****************************************************************************/

static ssize_t stm32_rng_read(struct file *filep, char *buffer,
                               size_t buflen)
{
  int ret;

  if (buflen == 0)
    {
      return 0;
    }

  ret = nxmutex_lock(&g_rngdev.rd_devlock);
  if (ret < 0)
    {
      return (ssize_t)ret;
    }

  /* Reset the semaphore for blocking until buffer is filled */

  nxsem_reset(&g_rngdev.rd_readsem, 0);

  g_rngdev.rd_buflen = buflen;
  g_rngdev.rd_buf = buffer;

  /* Enable RNG with interrupts */

  stm32_rng_enable();

  /* Wait until the buffer is filled */

  ret = nxsem_wait(&g_rngdev.rd_readsem);

  nxmutex_unlock(&g_rngdev.rd_devlock);

  return ret < 0 ? (ssize_t)ret : (ssize_t)buflen;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: devrandom_register
 *
 * Description:
 *   Initialize the RNG hardware and register the /dev/random driver.
 *   Must be called BEFORE devurandom_register.
 *
 ****************************************************************************/

#ifdef CONFIG_DEV_RANDOM
void devrandom_register(void)
{
  int ret;

  ret = stm32_rng_init_hw();
  if (ret < 0)
    {
      _err("ERROR: RNG hardware init failed: %d\n", ret);
      return;
    }

  ret = stm32_rng_initialize();
  if (ret < 0)
    {
      _err("ERROR: RNG interrupt init failed: %d\n", ret);
      return;
    }

  register_driver("/dev/random", &g_rngops, 0444, NULL);
}
#endif

/****************************************************************************
 * Name: devurandom_register
 *
 * Description:
 *   Register /dev/urandom backed by the hardware RNG.
 *
 ****************************************************************************/

#ifdef CONFIG_DEV_URANDOM_ARCH
void devurandom_register(void)
{
#ifndef CONFIG_DEV_RANDOM
  int ret;

  ret = stm32_rng_init_hw();
  if (ret < 0)
    {
      _err("ERROR: RNG hardware init failed: %d\n", ret);
      return;
    }

  ret = stm32_rng_initialize();
  if (ret < 0)
    {
      _err("ERROR: RNG interrupt init failed: %d\n", ret);
      return;
    }

#endif
  register_driver("/dev/urandom", &g_rngops, 0444, NULL);
}
#endif

#endif /* CONFIG_DEV_RANDOM || CONFIG_DEV_URANDOM_ARCH */
#endif /* CONFIG_STM32_RNG */
