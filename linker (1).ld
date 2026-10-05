/* task0/linker.ld - memory map for STM32F401CC (256 KB flash, 64 KB RAM).
 * Also fits the F411CE (512 KB / 128 KB); raise the sizes below to use it all. */
ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 256K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}

_estack = ORIGIN(RAM) + LENGTH(RAM);   /* stack grows down from end of RAM */

SECTIONS
{
    .isr_vector : { KEEP(*(.isr_vector)) } > FLASH   /* must be first in flash */

    .text : {
        *(.text*)
        *(.rodata*)
        . = ALIGN(4);
    } > FLASH

    _sidata = LOADADDR(.data);          /* flash address of .data's init image */
    .data : {
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss : {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    /DISCARD/ : { *(.ARM.exidx*) *(.comment) }
}
