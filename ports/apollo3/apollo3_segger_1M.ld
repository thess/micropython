/******************************************************************************
 *
 * Linker script for Micropython/FreeRTOS
 *
 *****************************************************************************/
ENTRY(Reset_Handler)

MEMORY
{
    ROMEM (rx) : ORIGIN = 0x0000C000, LENGTH = 1M
    RWMEM (rwx) : ORIGIN = 0x10000000, LENGTH = 384K
}

/* Total FLASH size (including bootcode) */
_flash_size = ORIGIN(ROMEM) + LENGTH(ROMEM);

SECTIONS
{
    .text :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        KEEP(*(.patch))
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
        _etext = .;
    } > ROMEM

    /* User stack section initialized by startup code. */
    .stack (NOLOAD):
    {
        . = ALIGN(8);
        *(.stack)
        *(.stack*)
        . = ALIGN(8);
    } > RWMEM

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RWMEM AT>ROMEM

    /* used by startup to initialize data */
    _init_data = LOADADDR(.data);

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RWMEM

    .ARM.attributes 0 : { *(.ARM.attributes) }

    /* Remaining RAM is heap */
    _ram_end_ = ORIGIN(RWMEM) + LENGTH(RWMEM) - 1 ;
    _heap_size = _ram_end_ - _ebss;

    .heap (NOLOAD):
    {
        . = ALIGN(8);
        _heap_start = .;
    } > RWMEM
}
