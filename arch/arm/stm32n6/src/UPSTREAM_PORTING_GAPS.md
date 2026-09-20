# STM32N6 Upstream Porting Gaps

This document tracks fixes and features from apache/nuttx upstream's
STM32N657 port that were evaluated during the CMSIS/upstream
alignment work but deliberately **not** ported into this repo's
drivers, along with the reason each was deferred. Nothing here is an
oversight -- each item was read, evaluated against this repo's actual
boot configuration, and either found inapplicable or too risky to
land without real-hardware verification.

Re-evaluate every item below once ADR-004 (real hardware validation)
moves from `PENDING` to `MEASURED`.

## 1. Chip-level header organization (stm32.h / chip.h aggregator)

**Upstream:** `stm32.h` is a single aggregator header that
`#include`s `stm32_gpio.h`, `stm32_lowputc.h`, `stm32_pwr.h`,
`stm32_rcc.h`, `stm32_uart.h` so driver `.c` files only need one
`#include "stm32.h"`. `chip.h` (src-level) forwards to
`hardware/stm32n6xxx_memorymap.h` and `<arch/stm32n6/chip.h>`.

**This repo:** each driver `.c` file directly `#include`s the
specific headers it needs; there is no aggregator.

**Why not ported:** purely a code-organization style difference with
no functional impact. Adopting the aggregator pattern would require
touching the `#include` block of every one of the 20+ driver files in
this tree for zero behavior change. Revisit only if a future large
refactor of this driver tree is already in scope for other reasons.

## 2. GPIO: dynamic per-pin bitfield macros

**Upstream:** `stm32_gpio.c`/`stm32_gpio.h` compute per-pin
MODER/OTYPER/OSPEEDR/PUPDR/AFR field positions with macros like
`GPIO_MODER_MASK(pin)` / `GPIO_MODER_SHIFT(pin)`.

**This repo:** keeps its existing compact 32-bit `cfgset` encoding
(`stm32n6_gpio.h`) with fixed shift constants computed inline in
`stm32n6_configgpio()`.

**Why not ported:** the *behavioral* fixes upstream's `stm32_gpio.c`
provides (spinlock, glitch-free output ordering, `-EINVAL`,
`unconfiggpio()`) were ported in isolation on top of this repo's
existing encoding (see `stm32n6_gpio.c`/`.h`, commit `7ce1302`).
Switching to upstream's macro-based encoding itself is a pure
refactor with no behavioral upside and would require re-encoding
every `GPIO_PORTx`/`GPIO_PIN`/`GPIO_AF` constant used across the
tree (including board.h pin assignments). Not worth the churn.

## 3. RCC: `STM32_APBxENSR`-style atomic peripheral clock enables for every peripheral

**Upstream:** `rcc_enableahb4()` in `stm32n6xx_rcc.c` enables *all*
AHB4 peripherals (every GPIO port + PWR) in a single
`STM32_RCC_AHB4ENSR` write, and `rcc_enableapb2()` conditionally
enables USART1 the same way.

**This repo:** `stm32n6_clockconfig()` only enables `GPIOEEN`
(USART1's PE5/PE6 pins) and `USART1EN` -- the minimum needed for the
console. Other peripheral drivers (SPI/I2C/FDCAN/SDMMC/XSPI/EMAC/
etc.) each independently enable their own clocks inside their own
`_initialize()` functions.

**Why not ported:** this repo's per-driver clock-enable pattern is
intentional and predates this porting effort; unconditionally
enabling every AHB4 GPIO port's clock at boot (whether or not any
driver uses that port) is a design choice upstream makes for a
simpler single-purpose board, not one this contest board's
broader driver set needs to copy. No functional bug here, just a
different (also valid) design.

## 4. RCC: `STM32_CPUCLK_FREQUENCY`-based SysTick reload -- PORTED

**Upstream:** `stm32_timerisr.c`'s `SYSTICK_RELOAD` macro uses
`STM32_CPUCLK_FREQUENCY` (a board.h macro) unconditionally.

**This repo (now):** `stm32n6_timerisr.c` uses `STM32_CPUCLK_FREQUENCY`,
matching upstream.  The prerequisite blocker below was resolved:
board.h's core-tree macros (`STM32_CPUCLK_FREQUENCY`/
`STM32_SYSCLK_FREQUENCY`/`STM32_HCLK_FREQUENCY`/`STM32_PCLK1`/`PCLK2`)
are now overridden to `STM32_HSI_FREQUENCY` when
`CONFIG_STM32N6_USE_PLL1` is not set, so the macro always describes the
clock source actually selected via Kconfig.  With the shipped default
(`USE_PLL1=n`) this collapses to 64 MHz, giving the correct 10 ms tick;
enabling PLL1 makes it track the 200/800 MHz target `stm32n6_clock-
config()` actually programs.

**Original blocker (resolved):** board.h previously defined
`STM32_CPUCLK_FREQUENCY` as the PLL1 *target* unconditionally, so using
it while PLL1 was disabled would have computed a 200 MHz reload against
a real 64 MHz clock -- a ~3.1x-too-slow tick (31.2 ms instead of
10 ms).

**Still to verify on real hardware (ADR-004):** confirm the tick rate
with a scope/logic analyzer on a GPIO toggled once per
`nxsched_process_timer()` call, in both `USE_PLL1=n` and `USE_PLL1=y`
builds.

## 5. `__start()`: naked dispatcher clearing MSPLIM/PSPLIM -- PORTED

**Upstream:** `__start()` is `naked` + `noinstrument_function` and
its first act is `msr msplim, r0` / `msr psplim, r0` (both zeroed)
before tail-calling `__start_c()`, because "the STM32N6 boot ROM (DEV
mode) leaves MSPLIM and PSPLIM set such that the first stack push
from C code can fault."

**This repo (now):** `__start()` is a `naked` + `noinstrument_function`
dispatcher that zeroes MSPLIM/PSPLIM and tail-calls `__start_c()`,
matching upstream.  The real boot logic moved into `__start_c()`.  As
part of the same change, `__start_c()` now points VTOR at the SRAM
vector table (`putreg32(_vectors, NVIC_VECTAB)`) as its first action,
closing the reset-to-`up_irqinitialize()` window during which VTOR
would otherwise still point at the boot ROM's table and dispatch any
early fault into ROM.

**Why the earlier "defer" reasoning was wrong:** the previous revision
of this doc deferred this fix on the grounds that the boot-time stack
fault had "never been observed."  That was circular -- the port had
only ever run under QEMU (mps3-an547) and this repo's Renode model,
neither of which emulates the STM32N6 DEV boot ROM, so neither leaves a
non-zero MSPLIM for the first C-code push to hit.  "Not observed in
simulators that cannot reproduce the trigger" is not evidence of
absence on real silicon.  Because `__start()` is a C function with
locals and calls, its compiler prologue pushes immediately, so a
non-zero ROM-set MSPLIM would fault before the first UART character --
a silent hang, since VTOR still pointed at ROM.  This is the single
most likely cause of a "flashed image produces no output" failure, and
the fix (naked prologue-free clear) is pure software with no downside
even if MSPLIM happens to already be zero.

**Still to verify on real hardware (ADR-004):** with a debugger at
reset, read MSPLIM/PSPLIM to confirm the ROM leaves them non-zero (the
assumption motivating the fix), and confirm the image now boots to the
NSH prompt.

## 6. `__start_c()`: SysTick disable + PENDSTCLR (FSBL chain-load compatibility)

**Upstream:** disables SysTick and clears any pending SysTick
interrupt at the very start of `__start_c()`, because "when
chain-loaded by an FSBL that called `HAL_Init()`, SysTick may be left
running."

**This repo:** no such handling.

**Why not ported:** this repo boots via **DEV mode with a debugger
writing directly to SRAM** -- there is no FSBL (First Stage Boot
Loader) in the boot path today. Confirmed via `docs/adr/ADR-004.md`
(still `PENDING`, not yet run on real hardware) and
`docs/adr/ADR-019.md` (which documents FSBL support as a *future*,
not-yet-integrated flash-boot path, separate from the current DEV
boot mode). The problem this code solves (stale FSBL-configured
SysTick state) has no trigger condition in this repo's current boot
path.

**What needs to happen before this can be revisited:** port this
fix if/when ADR-019's FSBL flash-boot path is actually integrated.
Until then it protects against a scenario that cannot occur.

## 7. CCR.LOB (Low-Overhead Branch) enable

**Upstream:** `stm32_enable_lob()` sets `NVIC_CFGCON.LOB` "before any
loop the compiler may lower with LE (and before MVE code, which is
gated on the same bit)."

**This repo:** no LOB handling; `NVIC_CFGCON_LOB` is not even defined
in this repo's headers.

**Why not ported:** LOB affects whether ARMv8.1-M WLS/DLS/LE
instructions (and MVE/Helium code) execute correctly. This repo does
not currently build any code path that intentionally emits MVE
instructions, so the risk of silently mis-executing a compiler-
generated LE loop is speculative rather than observed. Enabling a
CPU feature bit with no corresponding code that exercises it adds
complexity without a demonstrated need.

**What needs to happen before this can be revisited:** if/when this
repo starts building MVE-using code (e.g. NPU-adjacent DSP paths,
or if the compiler's `-mcpu=cortex-m55` output is confirmed to emit
LE-form loops that need this bit), port `stm32_enable_lob()` and
verify with a disassembly that the relevant loops actually execute
correctly with and without the bit set.

## 8. ES0620 / BSEC / SYSCFG erratum mitigations -- PARTIALLY PORTED

**Upstream:** `__start_c()` sets `RCC_APB4HENR_BSECEN` "per ES0620,
BSECEN must remain set or WFI/sleep fails," configures `LPEN` bits
so clocks keep running through WFI, calls
`stm32_pwr_enablevddio(BOARD_PWR_VDDIO)`, and writes
`SYSCFG_CCCR_ES0620_MANUAL` to `STM32_SYSCFG_VDDIO2CCCR`/
`VDDIO3CCCR`/`VDDCCCR` as an "ES0620 I/O-compensation mitigation."

**This repo -- what is now ported (WFI survival + VDDIO supply-valid):**
`__start_c()` now sets `RCC_APB4ENR2_BSECEN | RCC_APB4ENR2_SYSCFGEN`
(via the `APB4ENSR2` set alias) and the `BUSLPENR` (ACLKN/ACLKNC) +
`MEMLPENR` (all AXISRAM + CACHEAXIRAM) + `APB2LPENR` (USART1) LPEN
bits, and it now also calls `stm32n6_pwr_enablevddio(BOARD_PWR_VDDIO)`
before `stm32n6_lowsetup()` drives the console pins.  These were split
out from the rest of ES0620 and ported because they are *not*
speculative erratum mitigations -- they are hard boot prerequisites:

- The LPEN bits + BSECEN are required for `up_idle()`'s `WFI` to be
  survivable: this image runs from AXISRAM, and without them the
  RAM/bus clock stops during CSLEEP and the core never wakes from the
  SysTick interrupt.
- The `enablevddio()` call is required for the console to emit anything
  at all.  USART1's PE5/PE6 (AF7) are on GPIO port E, which the
  datasheet (DS14791 Table 18, notes 9/10) places on the VDDIO3/VDDIO2
  domains; those reset as not-supply-valid, so the pads cannot drive a
  level until the `PWR_SVMCR3` SV bits (in `BOARD_PWR_VDDIO`) are set.
  This is the same port-E-on-VddIO2/3 fact upstream's nucleo board.h
  documents, and our board.h uses a bit-identical `BOARD_PWR_VDDIO`.

(Note the register-naming difference from upstream: the BSEC/SYSCFG
enables live in `APB4ENR2` here, per CMSIS `RCC_APB4ENR2_*`, not
upstream's `APB4HENR` name.)

**This repo -- what is still NOT ported (the true I/O-compensation
erratum):** the `SYSCFG_CCCR_ES0620_MANUAL` writes to `VDDIO2CCCR`/
`VDDIO3CCCR`/`VDDCCCR`, and the `INITSVTORCR` secure-vector write.  The
SYSCFG CCCR registers are not even defined in this repo's headers yet.

**Why the I/O-compensation part stays deferred:** the VDDIO/CCCR writes
are a genuine silicon-errata mitigation whose applicability depends on
the exact silicon revision and this board's power/IO wiring, and a
wrong compensation value can *degrade* I/O timing rather than fix it.
Unlike the LPEN bits (which have an unambiguous, testable
failure mode -- WFI never wakes), the CCCR mitigation cannot be
validated without the errata sheet for the silicon on hand plus a real
board to measure I/O behaviour before/after.

**What needs to happen before the rest can be revisited:**
1. Confirm the exact STM32N647X0 silicon revision/date code on the
   real board, and check it against ST's ES0620 errata sheet.
2. If affected, define the `SYSCFG_*CCCR` registers, port
   `stm32_pwr_enablevddio(BOARD_PWR_VDDIO)`'s call site and the
   `SYSCFG_CCCR_ES0620_MANUAL` writes into `__start_c()`, and measure
   I/O compensation behaviour on hardware before/after.
3. If not affected, leave the I/O-compensation writes undone.

## 9. `.data` copy guard (partially ported, one difference remains)

**Upstream and this repo now both** skip the `.data` load-to-run
copy when `&_eronly[0] == &_sdata[0]` (ported in commit `0e97e28`).
This item is **not a gap** -- listed here only to note that the
guard is currently a no-op on this repo's SRAM-only DEV-boot linker
script (which always places `.data`'s load address ahead of its run
address), so it has not actually been exercised as a true skip-copy
path. Re-verify it actually skips the copy (not just compiles) if
this board's linker script is ever changed to a flash-boot layout
where the addresses could become equal.

## 10. RTC backup-domain write protection (`stm32n6_pwr_enablebkp`)

**Upstream `stm32_pwr_enablebkp()`** was ported as a standalone API
in commit `925a6fd` (PWR stage), but **`stm32n6_rtc.c` does not call
it** (or any equivalent) before writing RTC registers.

**Why not wired in:** the backup domain defaults to write-protected
on reset (`PWR_DBPCR.DBP` = 0), so if this repo's RTC driver has ever
successfully written its registers on real hardware, either (a) the
domain was already unlocked by something else in the boot path, or
(b) the writes have been silently dropped by hardware and RTC
functionality has never actually worked as intended. This can only
be distinguished with a real board: read back `PWR_DBPCR` after boot
and before any RTC configuration, or probe whether RTC time-keeping
actually persists/advances correctly.

**What needs to happen before this can be revisited:** on real
hardware, verify whether `stm32n6_rtc.c`'s configuration writes are
actually taking effect. If not, call `stm32n6_pwr_enablebkp(true)`
before RTC init in `stm32n6_rtc.c` (or once in `stm32n6_start.c`
before any RTC/backup-SRAM access) and re-verify.

## Summary table

| # | Item | File(s) | Blocker |
|---|------|---------|---------|
| 1 | stm32.h/chip.h aggregator headers | (organizational, all drivers) | None -- deliberately skipped, no bug |
| 2 | GPIO dynamic bitfield macros | stm32n6_gpio.h/.c | None -- deliberately skipped, no bug |
| 3 | Unconditional AHB4/APB2 clock enable-all | stm32n6_rcc.c | None -- different valid design |
| 4 | SysTick reload from STM32_CPUCLK_FREQUENCY | stm32n6_timerisr.c, board.h | board.h CPUCLK_FREQUENCY must track actual clock source |
| 5 | naked __start + MSPLIM/PSPLIM clear | stm32n6_start.c | Need debugger read of MSPLIM/PSPLIM at reset on real board |
| 6 | SysTick disable + PENDSTCLR (FSBL) | stm32n6_start.c | Need FSBL flash-boot path (ADR-019) actually integrated |
| 7 | CCR.LOB enable | stm32n6_start.c | Need MVE-using code path to exist first |
| 8 | ES0620/BSEC/SYSCFG erratum mitigation | stm32n6_start.c | Need silicon revision check + real-hardware WFI/sleep test |
| 10 | RTC backup-domain write unlock | stm32n6_rtc.c | Need real-hardware verification that RTC writes currently work or don't |
