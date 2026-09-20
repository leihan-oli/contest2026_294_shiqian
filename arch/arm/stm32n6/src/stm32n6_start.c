/****************************************************************************
 * arch/arm/src/stm32n6/stm32n6_start.c
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
#include <nuttx/cache.h>
#include <nuttx/init.h>

#include <stdint.h>

#include <arch/board/board.h>
#include <arch/barriers.h>

#include "arm_internal.h"
#include "nvic.h"
#include "hardware/stm32_rcc.h"
#include "hardware/stm32_pwr.h"
#include "hardware/stm32_syscfg.h"
#include "stm32n6_rcc.h"
#include "stm32n6_pwr.h"
#include "stm32n6_lowputc.h"

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

void stm32_boardinitialize(void);

/* __start_c() carries the real boot logic; __start() below is a naked
 * dispatcher that clears the boot-ROM stack limits before any compiler
 * prologue runs.  __start_c is reached via "b __start_c" from __start's
 * inline asm and therefore must have external linkage.
 */

void __start_c(void) noinstrument_function;

/****************************************************************************
 * SDMMC early probe (2026-09-08 diagnostic)
 *
 * Runs the exact same register sequence as the verified bare-metal golden
 * reference (baremetal_tfcard, [P] probe) in the EARLIEST NuttX
 * environment (after board init, BEFORE D-Cache/MPU/scheduler), to
 * bisect "environment difference": if this works but board_bringup
 * fails, the issue is in the later environment; if this also fails,
 * the issue is in the NuttX boot environment itself.
 ****************************************************************************/
/* SDMMC early-probe diagnostics removed 2026-09-10: the [E3]/[E3B]/
 * [E3C]/[E3D] probes and ep_* UART helpers were only needed to bisect
 * the SDMMC1 bring-up.  They are superseded by the verified driver
 * (see stm32n6_sdmmc.c) and the VDDIO4 fix below.
 */

#if 0
static void sdmmc_early_probe(const char *tag)
{
  volatile uint32_t *s = (volatile uint32_t *)0x58027000UL;
  uint32_t sta = 0;
  uint32_t i;

  ep_puts(tag);

  /* Environment snapshot (compare vs bare-metal tfcard [P] ENV) */
  {
    uint32_t control, primask, mspv, pspv;
    __asm__ __volatile__ ("mrs %0, CONTROL" : "=r" (control));
    __asm__ __volatile__ ("mrs %0, PRIMASK" : "=r" (primask));
    __asm__ __volatile__ ("mrs %0, MSP" : "=r" (mspv));
    __asm__ __volatile__ ("mrs %0, PSP" : "=r" (pspv));
    ep_puts(" ENV CTRL=");
    ep_hex32(control);
    ep_puts(" PRIMASK=");
    ep_hex32(primask);
    ep_puts(" MPU=");
    ep_hex32(getreg32(0xE000ED94UL));
    ep_puts(" CFGCON=");
    ep_hex32(getreg32(0xE000EDF0UL));
    ep_puts(" SAU=");
    ep_hex32(getreg32(0xE000EDD0UL));
    ep_puts(" ICSR=");
    ep_hex32(getreg32(0xE000ED04UL));
    ep_puts(" MSP=");
    ep_hex32(mspv);
    ep_puts(" PSP=");
    ep_hex32(pspv);
    ep_puts("\r\n");
  }

  /* enable SDMMC1 clock (AHB5ENSR) + HCLK source (CCIPR8=0) */
  putreg32(RCC_AHB5ENR_SDMMC1EN, STM32_RCC_BASE + 0x0A60UL); /* AHB5ENSR */
  modifyreg32(STM32_RCC_BASE + 0x160UL, 0x3UL, 0UL); /* CCIPR8 SDMMC1SEL=HCLK */

  /* CLKCR = CLKDIV 250 (1-bit) then POWER ON */
  s[0x04 >> 2] = 250;
  s[0x00 >> 2] = 3;

  /* wait > 74 cycles */
  for (i = 0; i < 1500000; i++) { }

  /* CMD0 */
  s[0x08 >> 2] = 0;
  s[0x0C >> 2] = (1u << 12);
  sta = 0;
  for (i = 0; i < 5000000; i++)
    {
      sta = s[0x34 >> 2];
      if ((sta & (1u << 7)) || (sta & (1u << 2))) break;
    }

  ep_puts(" S0=");
  ep_hex32(sta);

  if (sta & (1u << 7))
    {
      s[0x38 >> 2] = 0x8D;
    }

  /* CMD8 */
  s[0x08 >> 2] = 0x1AA;
  s[0x0C >> 2] = 8u | (1u << 8) | (1u << 12);
  sta = 0;
  for (i = 0; i < 5000000; i++)
    {
      sta = s[0x34 >> 2];
      if (((sta & 0x45) != 0) && !(sta & (1u << 13))) break;
    }

  ep_puts(" S8=");
  ep_hex32(sta);
  ep_putc('\r');
  ep_putc('\n');

  /* stop the block (power off) to not disturb later init */
  s[0x00 >> 2] = 0;
}
#endif


/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define IDLE_STACK \
  ((uintptr_t)_ebss + CONFIG_IDLETHREAD_STACKSIZE)

/****************************************************************************
 * Public Data
 ****************************************************************************/

const uintptr_t g_idle_topstack = IDLE_STACK;

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: showprogress
 *
 * Description:
 *   Print a character on the UART to show boot progress.
 *
 ****************************************************************************/

#ifdef CONFIG_DEBUG_FEATURES
static inline void showprogress(char c)
{
  arm_lowputc(c);
}
#else
#  define showprogress(c)
#endif

/****************************************************************************
 * Name: dbg_led0_blink
 *
 * Description:
 *   Flash-boot bring-up marker on LED0 (PG10, active-low).  Uses direct
 *   register access on the GPIOG NS alias so it works from the very first
 *   instruction of __start_c, before any NuttX driver exists.  The FSBL
 *   already enabled the GPIOG clock and configured the pin, and it is
 *   still on when we land here.  Blink N times then leave LED0 ON.
 *
 ****************************************************************************/

static void dbg_led0_blink(int n)
{
  volatile uint32_t *bsrr;
  int i;
  volatile uint32_t d;

  /* GPIOG SECURE alias, BSRR @ +0x18.  PG10: ON=BR10(bit26), OFF=BS10(bit10).
   * Must use the secure alias: the FSBL (secure world) owns the
   * peripherals and the NS alias of a secure peripheral faults. */

  bsrr = (volatile uint32_t *)(0x56021800UL + 0x18UL);
  for (i = 0; i < n; i++)
    {
      *bsrr = (1UL << 26);              /* LED0 ON */
      for (d = 0; d < 3000000UL; d++) { __asm__ volatile("nop"); }
      *bsrr = (1UL << 10);              /* LED0 OFF */
      for (d = 0; d < 3000000UL; d++) { __asm__ volatile("nop"); }
    }

  *bsrr = (1UL << 26);                  /* leave LED0 ON */
}

/****************************************************************************
 * Name: stm32n6_enable_lob
 *
 * Description:
 *   Enable the Cortex-M55 ARMv8.1-M Low-Overhead Branch extension
 *   (CCR.LOB).  This gates the WLS/DLS/LE loop instructions the compiler
 *   may emit and the MVE data path.  CCR.LOB resets to 0, so it must be
 *   set before any loop the compiler could lower with LE runs.  Matches
 *   upstream stm32_start.c stm32_enable_lob().
 *
 ****************************************************************************/

static inline void stm32n6_enable_lob(void)
{
  uint32_t regval;

  regval  = getreg32(NVIC_CFGCON);
  regval |= NVIC_CFGCON_LOB;
  putreg32(regval, NVIC_CFGCON);
  UP_ISB();
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: __start
 *
 * Description:
 *   Reset entry point.  In DEV boot the STM32N6 boot ROM does not perform a
 *   Cortex-M reset sequence into our image: it *branches* to _stext with
 *   MSP still pointing at the boot ROM's own high stack region, and with
 *   MSPLIM/PSPLIM left set such that the first stack push from C code can
 *   fault.  Because MSP is never reloaded from _vectors[0] the way a real
 *   reset would, the running stack sits ~800 KiB above g_idle_topstack.
 *   arm_stack_color(idle_stack, 0) then colours everything from the idle
 *   stack base up to the *current* SP with STACK_COLOR (0xdeadbeef),
 *   overwriting the heap metadata that lives at g_idle_topstack and
 *   corrupting delay.head -> UNALIGNED UsageFault on the first malloc.
 *
 *   This function is naked so MSP can be reloaded to our idle-thread stack
 *   top and the stack limits cleared before any compiler-generated
 *   prologue runs, then it tail-calls __start_c.  On a platform that does
 *   reset normally (e.g. Renode) MSP already equals g_idle_topstack, so the
 *   reload is a harmless no-op.
 *
 *   FLASH-BOOT (FSBL) PATH: the FSBL jumps to _vectors[1] with MSP loaded
 *   from _vectors[0], so start() runs first and calls arm_initialize_stack(),
 *   which sets CONTROL.SPSEL=1 (thread mode uses PSP=g_idle_topstack) and
 *   MSP=interrupt stack.  We MUST then NOT reload MSP with g_idle_topstack:
 *   that would make MSP == PSP, so every interrupt handler (which runs on
 *   MSP) would clobber the task context that exception entry saved on PSP;
 *   the first context switch back to the interrupted task would then
 *   restore a corrupted PC (e.g. g_intstacktop) and IBUSERR HardFault.
 *   Hence the CONTROL.SPSEL check below: only take the MSP=g_idle_topstack
 *   path when SPSEL==0 (DEV-boot, where start()/arm_initialize_stack()
 *   never ran and thread mode uses MSP).
 *
 ****************************************************************************/

void __attribute__((naked)) noinstrument_function __start(void)
{
  __asm__ volatile ("mrs r2, CONTROL\n\t"
                    "tst r2, #2\n\t"
                    "bne 2f\n\t"
                    "mov r0, #0\n\t"
                    "msr msplim, r0\n\t"
                    "msr psplim, r0\n\t"
                    "ldr r0, =g_idle_topstack\n\t"
                    "ldr r0, [r0]\n\t"
                    "msr msp, r0\n\t"
                    "isb\n\t"
                    "2:\n\t"
                    /* DIAG v5: light LED0 through the SECURE GPIOG BSRR
                     * alias (0x56021818, BR10 = bit26 = LED0 ON).  MUST use
                     * the secure alias: the FSBL leaves the GPIO pins in
                     * their reset SECURE state (no ConfigPinAttributes
                     * call), and a write to the NS alias of a secure
                     * peripheral SecureFaults (this previously made the
                     * board look like the CPU never reached __start, i.e.
                     * "fully dark" even though the jump had succeeded).
                     *   LED0 solid ON  -> __start ran.
                     *   LED0 blinks 1x -> __start_c's dbg_led0_blink(1)
                     *     (also secure alias) worked; the hang is later
                     *     (putreg32/clockconfig).
                     *   LED0 never lit -> CPU never reached __start; the
                     *     hang is in start()/arm_initialize_stack() or the
                     *     FSBL jump vector was wrong. */
                    "movw r1, #0x1818\n\t"
                    "movt r1, #0x5602\n\t"
                    "movs r2, #1\n\t"
                    "lsls r2, r2, #26\n\t"
                    "str r2, [r1, #0]\n\t"
                    "b __start_c\n\t");
}

/****************************************************************************
 * Name: __start_c
 *
 * Description:
 *   The C-level boot path, reached from the naked __start dispatcher.
 *
 ****************************************************************************/

void __start_c(void)
{
  const uint32_t *src;
  uint32_t *dest;

  /* Marker 1: NuttX __start_c is executing, i.e. the FSBL->NuttX jump
   * itself succeeded (LED0 blinks 1x and stays ON).  Later boot progress
   * is reported on the UART via showprogress('A'..'D') after lowsetup().
   */

  dbg_led0_blink(1);

  /* NOTE (2026-09-09): the [E1]/[E2] SDMMC diagnostic probes and the
   * SPSEL=0 experiment were REMOVED.  They ran before the SDMMC GPIO was
   * configured, so CMD0/CMD8 never completed, leaving SDMMC STA.CPSMACT
   * stuck (0x2000) which polluted the real driver's initialization
   * (its init dump then showed STA=0x2000 and GO_IDLE always timed out).
   * The bare-metal tfcard [Z] experiment proved the NuttX register
   * sequence is correct when the environment is clean.
   */

  /* The DEV-mode boot ROM leaves VTOR pointing at its own ROM region.
   * Point VTOR at our SRAM vector table before any exception path can
   * run, so an early fault is dispatched to our handlers (which decode
   * it) rather than silently into the boot ROM's vectors.  up_irqinit-
   * ialize() sets this again later; setting it here closes the window
   * from reset until NuttX runs.
   */

  putreg32((uint32_t)_vectors, NVIC_VECTAB);

  /* When chain-loaded by an FSBL that called HAL_Init(), SysTick may be
   * left running.  Disable it and clear any pending SysTick interrupt so
   * it does not fire before NuttX has attached its handler.  Matches
   * upstream stm32_start.c.
   */

  putreg32(0, NVIC_SYSTICK_CTRL);
  putreg32(NVIC_INTCTRL_PENDSTCLR, NVIC_INTCTRL);

  /* Force plain SLEEP (not DEEPSLEEP) on WFI so the system clock keeps
   * running and SysTick continues to wake us.  Cleared once here so
   * up_idle()'s WFI stays a shallow sleep on every idle entry.  (Resets
   * to 0 on this core, but clear it explicitly so the boot state does
   * not depend on a chain-loader leaving it untouched.)
   */

  modifyreg32(NVIC_SYSCON, NVIC_SYSCON_SLEEPDEEP, 0);

  /* Enable the FPU before stm32n6_clockconfig and the rest of init.  With
   * the hard-float ABI the compiler may emit FPU instructions later, and
   * any exception entry will try to push FP context -- both require
   * CP10/CP11 to be enabled.  arm_fpuconfig() is a no-op unless
   * CONFIG_ARCH_FPU is set.
   */

  arm_fpuconfig();

  /* Clear .bss */

  for (dest = (uint32_t *)_sbss; dest < (uint32_t *)_ebss; )
    {
      *dest++ = 0;
    }

  /* Copy .data from flash to SRAM.
   *
   * Ported from apache/nuttx upstream stm32_start.c: skip the copy
   * if _eronly and _sdata are the same address.  This repo's DEV
   * boot (SRAM-only, no XSPI flash, no FSBL) linker script
   * currently always places .data's load address ahead of its
   * run address, so this check is presently a no-op guard rather
   * than an active optimization -- but it is cheap, correct in
   * both cases, and protects against a future linker script
   * change (e.g. adding a flash-boot target) that made them equal
   * without this guard, which would otherwise copy .data onto
   * itself or read past a zero-length source region.
   */

  if (&_eronly[0] != &_sdata[0])
    {
      src = (const uint32_t *)_eronly;
      dest = (uint32_t *)_sdata;
      for (; dest < (uint32_t *)_edata; )
        {
          *dest++ = *src++;
        }
    }

  /* Enable the Cortex-M55 Low-Overhead-Branch extension before any code
   * that the compiler may have lowered with LE/WLS/DLS runs.
   */

  stm32n6_enable_lob();

  /* Configure clocks */

  stm32n6_clockconfig();

  /* Per ES0620, BSECEN must remain set or WFI/sleep fails.  Set it via
   * the atomic APB4ENSR2 set alias, together with SYSCFGEN which the
   * SYSCFG-based erratum mitigations would need.
   */

  putreg32(RCC_APB4ENR2_BSECEN | RCC_APB4ENR2_SYSCFGEN,
           STM32_RCC_APB4ENSR2);

  /* Enable the LPEN bits that keep clocks running through WFI (CSLEEP).
   * Without these, WFI halts the bus/RAM clocks for the AXISRAM banks
   * this image runs from, and the core never wakes from the SysTick
   * interrupt.
   */

  putreg32(RCC_BUSLPENR_ACLKNLPEN | RCC_BUSLPENR_ACLKNCLPEN,
           STM32_RCC_BUSLPENSR);
  putreg32(RCC_MEMLPENR_ALLAXISRAM | RCC_MEMLPENR_CACHEAXIRAMLPEN,
           STM32_RCC_MEMLPENSR);
#ifdef CONFIG_STM32_USART1
  putreg32(RCC_APB2LPENR_USART1LPEN, STM32_RCC_APB2LPENSR);

  /* Route USART1's kernel clock to HSI so the BRR computation in
   * stm32n6_lowsetup() is independent of any later SYSCLK change (e.g.
   * enabling PLL1).  Use read-modify-write to preserve the other fields
   * in CCIPR13.
   */
  {
    uint32_t regval;

    regval  = getreg32(STM32_RCC_CCIPR13);
    regval &= ~RCC_CCIPR13_USART1SEL_MASK;
    regval |= RCC_CCIPR13_USART1SEL_HSI;
    putreg32(regval, STM32_RCC_CCIPR13);
  }
#endif

  /* Mark the board's I/O voltage domains as supply-valid before any GPIO
   * pad is driven.  The USART1 console pins PE5/PE6 (AF7) are on GPIO
   * port E, which the datasheet (DS14791 Table 18 notes 9/10) places on
   * the VDDIO3/VDDIO2 domains; those domains reset as not-supply-valid,
   * so their pads cannot drive a level until the matching SV bits are
   * set.  Without this the CPU boots but the UART emits nothing.  The
   * PWR_SVMCR3_* mask is board-specific and supplied by board.h via
   * BOARD_PWR_VDDIO.  (PWR is on an always-on domain and needs no clock
   * gate, matching upstream's call site.)
   */

  stm32n6_pwr_enablevddio(BOARD_PWR_VDDIO);

  /* VDDIO4 supply-valid (ROOT CAUSE of the SDMMC1/TF-card failure,
   * 2026-09-09): DS14791/AN5967 place PC[12:6], PC1 and PH[9,2] -- the
   * SDMMC1 pins PC8-12/PH2 among them -- on the VDDIO4 domain, whose
   * pads cannot drive a level until PWR_SVMCR1.VDDIO4SV is set.
   * BOARD_PWR_VDDIO only declares VDDIO2/VDDIO3 (console/LTDC/DCMIPP
   * ports), so the SDMMC CMD0 never completed (CPSMACT frozen) even
   * though every register/GPIO/clock value matched the verified
   * bare-metal reference.  The bare-metal 38_SD_Card HAL_MspInit()
   * calls HAL_PWREx_EnableVddIO4() (3.3V: VDDIO4VRSEL=0) -- mirror it:
   * clear VRSEL first, then set SV.
   */

  modifyreg32(STM32_PWR_SVMCR1, PWR_SVMCR1_VDDIO4VRSEL, 0); /* 3.3V range */
  modifyreg32(STM32_PWR_SVMCR1, 0, PWR_SVMCR1_VDDIO4SV);

  /* Apply the ES0620 I/O-compensation mitigation (write 0x287) to the
   * domains we use.  Only VDDIO2 and VDDIO3 are touched: the other
   * VDDIOxCCCR registers cannot be accessed without their VDDIOxSV bit
   * set first (a separate ES0620 constraint), and only VDDIO2/3 are
   * declared supply-valid in BOARD_PWR_VDDIO above.  The register
   * offsets come from ST CMSIS stm32n647xx.h (VDDIO2CCCR=0x44,
   * VDDIO3CCCR=0x4c), not from upstream nuttx (which mislabels 0x54/0x5c
   * -- those are VDDIO4/5 on this silicon).  See stm32_syscfg.h.
   */

  putreg32(SYSCFG_CCCR_ES0620_MANUAL, STM32_SYSCFG_VDDIO2CCCR);
  putreg32(SYSCFG_CCCR_ES0620_MANUAL, STM32_SYSCFG_VDDIO3CCCR);
  putreg32(SYSCFG_CCCR_ES0620_MANUAL, STM32_SYSCFG_VDDCCCR);

  /* Point the Cortex-M55 secure vector-table base at our SRAM vectors,
   * matching upstream stm32_start.c.
   */

  putreg32((uint32_t)_vectors, STM32_SYSCFG_INITSVTORCR);

  /* Read-back to ensure the prior SYSCFG writes have completed before we
   * start driving pads.
   */

  (void)getreg32(STM32_SYSCFG_VDDCCCR);

  /* Configure the UART for early debug output.  From this point on
   * showprogress() can emit characters, so the markers below let us
   * pinpoint the boot stage by the last character printed.
   */

  stm32n6_lowsetup();

  showprogress('A');

  /* Call board early initialization */

  stm32_boardinitialize();
  showprogress('B');

  /* Enable the D-Cache.  The FSBL leaves SCTLR.C=0, so every SRAM access
   * (LVGL rendering into the draw buffer, the framebuffer memcpy, ...)
   * went straight to the bus with no caching -- measured 8KB fb write =
   * ~303K cycles vs ~7K expected.  That is why the LCD putarea memcpy
   * costs ~19ms for 160KB and full-screen LVGL drags run at 2-5 fps.
   * up_enable_dcache() invalidates first, then sets SCTLR.C.
   * SRAM (0x34000000+) is Normal/cacheable in the default ARMv8-M memory
   * map and peripherals (0x50000000+) are Device (never cached), so this
   * is safe.  The LCD putarea flushes the written framebuffer region
   * afterwards, so the LTDC DMA still reads the latest pixels. */

  /* NOTE (2026-09-11): running with the D-Cache disabled was tried as a
   * suspected cause of random heap corruption (mm.h:354).  The corruption
   * reproduced unchanged with the cache off, so the cache is NOT the cause
   * and it is re-enabled here for the rendering performance it buys.
   * See the repo memory note "atk-dnn647-dcache-heap-corruption" for the
   * list of suspects already ruled out.
   */

  up_enable_dcache();

  /* Same for the I-Cache, guarded because the option is per-config and
   * the other ATON-DNN647 builds do not set it.
   */

#ifdef CONFIG_ARMV8M_ICACHE
  up_enable_icache();
#endif

  showprogress('K');

#ifdef USE_EARLYSERIALINIT
  arm_earlyserialinit();
#endif
  showprogress('C');

  /* Barrier after the SCB (VTOR), SYSCON and SYSCFG (INITSVTORCR) writes
   * above before handing off to NuttX.
   */

  UP_DSB();
  UP_ISB();

  /* 'D' means __start_c ran to completion; anything that faults after
   * this marker is inside nx_start() (heap init, up_irqinitialize,
   * up_timer_initialize, board late init, ...).
   */

  showprogress('D');

  /* Start NuttX */

  showprogress('\r');
  showprogress('\n');
  nx_start();

  for (; ; );
}
