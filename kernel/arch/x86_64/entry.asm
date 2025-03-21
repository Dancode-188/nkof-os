;
; NKOF Kernel Entry Point
;
; This is the entry point for the kernel, called by the bootloader.
; It sets up the environment for the C kernel and calls the main function.

[BITS 32]

; Export kernel_main symbol (defined in C)
extern kernel_main

; Export our entry point to the linker
global kernel_entry

; Multiboot header for compatibility (not required, but useful)
section .multiboot
align 4
    dd 0x1BADB002           ; Magic number
    dd 0x00                 ; Flags
    dd -(0x1BADB002 + 0x00) ; Checksum

section .text
kernel_entry:
    ; Set up kernel stack
    mov esp, kernel_stack_top

    ; Clear interrupts
    cli

    ; Initialize essential CPU state here if needed

    ; Call the C kernel main function
    call kernel_main

    ; If kernel_main returns, halt the CPU
.hang:
    cli                     ; Disable interrupts
    hlt                     ; Halt the CPU
    jmp .hang               ; Just in case an interrupt wakes up the CPU

section .bss
align 16
kernel_stack_bottom:
    resb 16384             ; 16 KB for kernel stack
kernel_stack_top: