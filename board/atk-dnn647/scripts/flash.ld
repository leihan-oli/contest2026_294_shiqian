/****************************************************************************
 * vendor/openvela/boards/atk-dnn647/scripts/flash.ld
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

/* The STM32N647X0 has no internal flash.  In DEV boot mode, code is loaded
 * directly into SRAM by the ST-Link debugger at 0x34000400.  The first
 * 1 KiB of SRAM is reserved for the boot ROM header.
 */

MEMORY
{
    sram (rwx) : ORIGIN = 0x34000400, LENGTH = 4193280

    /* Non-secure window for the epoch program buffers: the free tail of the
     * NPU's own RAM (SRAM3..6), reached through its alias.
     *
     * The NPU is a non-secure master, so every buffer it touches has to live
     * in a RISAF region configured non-secure.  The RAM that is free and
     * unguarded by RISAF2/RISAF3 (the instances that also guard the NuttX
     * heap, and which cannot be carved safely) is the top of the model's
     * vpool: the generated model only ever addresses up to
     * 0x24200000 + 0xE2480, so 0x24300000..0x243C0000 is untouched.
     */

    /* 2026-09-17: LENGTH widened from 147456 (144 KB) to 622592 (608 KB).
     *
     * The eye (blink 64x128) epoch program is 172 KB - bigger than pose994's
     * 139 KB, which is what the original 144 KB was sized for - so the
     * original window overflowed by 28448 bytes.  The comment above already
     * documents that 0x24300000..0x243C0000 is untouched, so the remaining
     * 0x2434C000..0x243C0000 is free to use.
     *
     * Keep the LENGTH in sync with STM32N6_ATON_NS_WINDOW_SIZE in
     * arch/arm/src/stm32n6/stm32n6_aton_aie.c.
     */

    nsram (rw) : ORIGIN = 0x24328000, LENGTH = 622592
}

OUTPUT_ARCH(arm)
ENTRY(_stext)
SECTIONS
{
    .text : {
        _stext = ABSOLUTE(.);
        *(.vectors)
        *(.text .text.*)
        *(.fixup)
        *(.gnu.warning)
        *(.rodata .rodata.*)
        *(.gnu.linkonce.t.*)
        *(.glue_7)
        *(.glue_7t)
        *(.got)
        *(.gcc_except_table)
        *(.gnu.linkonce.r.*)
        _etext = ABSOLUTE(.);
    } > sram

    .init_section : {
        _sinit = ABSOLUTE(.);
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
        KEEP(*(.init_array EXCLUDE_FILE(*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o) .ctors))
        _einit = ABSOLUTE(.);
    } > sram

    .ARM.extab : {
        *(.ARM.extab*)
    } > sram

    __exidx_start = ABSOLUTE(.);
    .ARM.exidx : {
        *(.ARM.exidx*)
    } > sram
    __exidx_end = ABSOLUTE(.);

    .data : {
        _eronly = ABSOLUTE(.);
        _sdata = ABSOLUTE(.);
        *(.data .data.*)
        *(.gnu.linkonce.d.*)
        CONSTRUCTORS
        . = ALIGN(4);
        _edata = ABSOLUTE(.);
    } > sram

    .bss : {
        _sbss = ABSOLUTE(.);
        *(.bss .bss.*)
        *(.gnu.linkonce.b.*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = ABSOLUTE(.);
    } > sram

    /* Epoch Controller program buffers (the ECBLOB_RUNTIME_SECTION arrays
     * emitted by the generated network_ecblobs.h).  NOLOAD: the buffers are
     * filled at runtime by ec_copy_program(), and keeping 140 KB of zeros in
     * the image would push it past the FSBL's 1 MiB LRUN window.  The NS
     * window is zeroed once from the ATON driver before use.
     */

    .nsblob (NOLOAD) : ALIGN(32) {
        _snsblob = ABSOLUTE(.);
        *(.nsblob)
        *(.nsblob.*)
        . = ALIGN(32);
        _ensblob = ABSOLUTE(.);
    } > nsram

    /* Stabs debugging sections. */

    .stab 0 : { *(.stab) }
    .stabstr 0 : { *(.stabstr) }
    .stab.excl 0 : { *(.stab.excl) }
    .stab.exclstr 0 : { *(.stab.exclstr) }
    .stab.index 0 : { *(.stab.index) }
    .stab.indexstr 0 : { *(.stab.indexstr) }
    .comment 0 : { *(.comment) }
    .debug_abbrev 0 : { *(.debug_abbrev) }
    .debug_info 0 : { *(.debug_info) }
    .debug_line 0 : { *(.debug_line) }
    .debug_pubnames 0 : { *(.debug_pubnames) }
    .debug_aranges 0 : { *(.debug_aranges) }

    /* CRITICAL for flash-boot (LRUN): drop the GNU build-id note.  The
     * default .note.gnu.build-id lands at the memory ORIGIN (0x34000400)
     * and pushes .text/.vectors to 0x34000800.  The LRUN bootloader copies
     * the image to a fixed 0x34000400, so a build-id note causes a 0x400
     * misalignment: the vector table ends up at the wrong address and the
     * CPU executes garbage after the jump (LEDs freeze, no UART output).
     * DEV-boot worked because the debugger places code at the link
     * addresses; flash-boot cannot. */
    /DISCARD/ : {
        *(.note.gnu.build-id)
    }
}
