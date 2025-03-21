/* 
 * NKOF Kernel Linker Script
 *
 * This script tells the linker how to construct the kernel binary.
 */

OUTPUT_FORMAT("elf32-i386")
ENTRY(kernel_entry)

SECTIONS
{
    /* The kernel will be loaded at 1MB physical */
    . = 1M;

    /* Multiboot header needs to be early */
    .multiboot : {
        *(.multiboot)
    }

    /* Code section */
    .text : {
        *(.text)
    }

    /* Read-only data */
    .rodata : {
        *(.rodata)
    }

    /* Initialized data */
    .data : {
        *(.data)
    }

    /* Uninitialized data (will be zeroed) */
    .bss : {
        *(.bss)
    }
    
    /* Export the end of kernel symbol */
    end = .;

    /* Discard unnecessary sections */
    /DISCARD/ : {
        *(.comment)
        *(.eh_frame)
    }
}