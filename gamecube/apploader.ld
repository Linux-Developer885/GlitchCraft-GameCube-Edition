OUTPUT_FORMAT("elf32-powerpc")
ENTRY(apploader_entry)

SECTIONS
{
    . = 0x81200000;
    .text : {
        KEEP(*(.text.apploader_entry))
        *(.text*)
        *(.rodata*)
    }
    .data : {
        *(.data*)
        *(.sdata*)
    }
    .bss : {
        *(.bss*)
        *(COMMON)
    }
    /DISCARD/ : {
        *(.comment*)
        *(.eh_frame*)
        *(.note*)
        *(.rela*)
    }
}
