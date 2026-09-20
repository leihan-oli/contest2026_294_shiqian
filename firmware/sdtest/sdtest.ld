/* sdtest.ld -- minimal SRAM image, same layout as the NuttX image the
 * FSBL copies and jumps to (vector table + entry at 0x34000400, stack
 * top just below the FSBL's 0x34200000 MSP check bound). */

ENTRY(Reset_Handler)

MEMORY
{
  sram (rwx) : ORIGIN = 0x34000400, LENGTH = 4193280
}

SECTIONS
{
  .text :
  {
    KEEP(*(.isr_vector))
    *(.text*)
    *(.rodata*)
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > sram

  /* -mcmse secure gateway veneers must live in their own output section */
  .gnu.sgstubs :
  {
    . = ALIGN(4);
    *(.gnu.sgstubs*)
    . = ALIGN(4);
  } > sram

  .ARM :
  {
    *(.ARM.exidx*)
  } > sram

  .data :
  {
    _sdata = .;
    *(.data*)
    _edata = .;
    _sidata = _sdata;   /* no LMA separation: image is copied as-is */
  } > sram

  .bss :
  {
    _sbss = .;
    *(.bss*)
    *(COMMON)
    _ebss = .;
  } > sram

  . = ALIGN(8);
  _estack = 0x341FF000;      /* top of stack (FSBL wants MSP < 0x34200000) */
  _sstack = _estack - 0x4000; /* MSPLIM = low stack bound (16 KB space); the
                                 startup.s sets MSPLIM=_sstack then SP=_estack,
                                 so MSPLIM must sit BELOW SP or the first
                                 exception push (SysTick) faults */
}
