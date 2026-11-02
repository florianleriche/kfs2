OUTPUT_FORMAT(elf32-i386)
OUTPUT_ARCH(i386)
ENTRY(_start)

SECTIONS
{
    . = 1M; /* adresse de chargement attendue par GRUB */

    .multiboot ALIGN(4) :
    {
        KEEP(*(.multiboot)) /* doit rester tout au début du binaire pour que GRUB le trouve */
    }

    .text ALIGN(4K) :
    {
        *(.text*)
    }

    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    }

    .data ALIGN(4K) :
    {
        *(.data*)
    }

    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss*)
    }
}
