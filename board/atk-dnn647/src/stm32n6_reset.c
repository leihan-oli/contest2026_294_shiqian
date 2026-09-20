/****************************************************************************
 * boards/arm/stm32n6/atk-dnn647/src/stm32n6_reset.c
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

#include <nuttx/arch.h>
#include <nuttx/board.h>

#include "arm_internal.h"
#include "hardware/stm32_rcc.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_reset_cause
 *
 * Description:
 *   Report the cause of the last reset by decoding the sticky flags in the
 *   RCC reset status register (RSR), then clear them (RSR.RMVF) so the next
 *   reset reports a fresh cause.  Clearing matters because the power-on flag
 *   stays latched across later resets and would otherwise mask a subsequent
 *   watchdog reset.
 *
 *   On STM32N6 the CPU/application reads its reset cause from RSR (0x34);
 *   the sibling HWRSR (0x30) mirrors the hardware power domain and reads 0
 *   from the CPU/debug AP (measured on real silicon: after an IWDG reset RSR
 *   held IWDGRSTF while HWRSR was 0).  Decoding therefore uses RSR.
 *
 *   The flags are decoded in priority order: watchdog first (the reason a
 *   watchdog test cares about), then software, pin, brown-out and finally
 *   power-on, which is the baseline flag set on every cold boot.
 *
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_RESET_CAUSE
int board_reset_cause(struct boardioc_reset_cause_s *cause)
{
  uint32_t rsr = getreg32(STM32_RCC_RSR);

  if ((rsr & (RCC_RSR_IWDGRSTF | RCC_RSR_WWDGRSTF)) != 0)
    {
      /* Either watchdog reset maps to the generic system-watchdog cause */

      cause->cause = BOARDIOC_RESETCAUSE_SYS_RWDT;
    }
  else if ((rsr & RCC_RSR_SFTRSTF) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_CPU_SOFT;
    }
  else if ((rsr & RCC_RSR_PINRSTF) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_PIN;
    }
  else if ((rsr & RCC_RSR_LPWRRSTF) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_LOWPOWER;
    }
  else if ((rsr & RCC_RSR_BORRSTF) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_SYS_BOR;
    }
  else if ((rsr & RCC_RSR_PORRSTF) != 0)
    {
      cause->cause = BOARDIOC_RESETCAUSE_SYS_CHIPPOR;
    }
  else
    {
      cause->cause = BOARDIOC_RESETCAUSE_UNKOWN;
    }

  /* Clear the sticky flags so the next reset reports a fresh cause */

  putreg32(RCC_RSR_RMVF, STM32_RCC_RSR);

  return 0;
}
#endif /* CONFIG_BOARDCTL_RESET_CAUSE */

/****************************************************************************
 * Name: board_reset
 *
 * Description:
 *   Reset the board.  Required by board-level logic when
 *   CONFIG_BOARDCTL_RESET is selected.
 *
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_RESET
int board_reset(int status)
{
  UNUSED(status);
  up_systemreset();
  return 0;
}
#endif /* CONFIG_BOARDCTL_RESET */
