;
; NKOF Stage 1 Bootloader
; 
; This bootloader is the first code executed by the BIOS after power-on.
; It's responsible for:
; 1. Basic initialization
; 2. Printing a welcome message
; 3. Checking for disk extensions
; 4. Loading the second stage bootloader

[BITS 16]           ; Operating in 16-bit Real Mode
[ORG 0x7C00]        ; BIOS loads the boot sector to memory address 0x7C00

;--------------------------------------------------
; Bootloader Entry Point
;--------------------------------------------------
start:
    ; Initialize segment registers
    xor ax, ax      ; Set AX to 0 (faster than mov ax, 0)
    mov ds, ax      ; Data Segment = 0
    mov es, ax      ; Extra Segment = 0
    mov ss, ax      ; Stack Segment = 0
    mov sp, 0x7C00  ; Set up stack below our code to prevent overwriting

    ; Save boot drive number (provided by BIOS in DL)
    mov [DriveId], dl

    ; Print welcome message
    mov si, WelcomeMsg
    call PrintString

    ; Check for disk extensions
    call CheckDiskExtensions

    ; Halt the system (for now - will be replaced with stage 2 loading)
    jmp HaltSystem

;--------------------------------------------------
; Function: PrintString
; Input: SI = Pointer to null-terminated string
;--------------------------------------------------
PrintString:
    push ax         ; Save registers we'll modify
    push bx
    
    mov ah, 0x0E    ; BIOS teletype function
    mov bx, 0x07    ; Page 0, text attribute 7 (light gray)
    
.loop:
    lodsb           ; Load next character from SI into AL and increment SI
    test al, al     ; Check if character is 0 (end of string)
    jz .done        ; If zero, we're done
    
    int 0x10        ; Print character in AL
    jmp .loop       ; Repeat for next character
    
.done:
    pop bx          ; Restore registers
    pop ax
    ret

;--------------------------------------------------
; Function: CheckDiskExtensions
; Checks if BIOS disk extensions (LBA support) are available
;--------------------------------------------------
CheckDiskExtensions:
    mov ah, 0x41            ; Function 41h - Check Extensions Present
    mov bx, 0x55AA          ; Required signature
    mov dl, [DriveId]       ; Drive to check
    int 0x13                ; BIOS disk interrupt
    
    jc .not_supported       ; Carry flag set = error/not supported
    cmp bx, 0xAA55          ; BX should be adjusted to 0xAA55
    jne .not_supported
    
    ; Extensions are supported
    mov si, DiskExtMsg
    call PrintString
    stc                     ; Set carry flag indicating success
    ret
    
.not_supported:
    mov si, NoDiskExtMsg
    call PrintString
    clc                     ; Clear carry flag indicating failure
    ret

;--------------------------------------------------
; Function: HaltSystem
; Halts the CPU until the next interrupt, then repeats
;--------------------------------------------------
HaltSystem:
    mov si, HaltMsg
    call PrintString
    
.halt_loop:
    hlt                     ; Halt the CPU
    jmp .halt_loop          ; Just in case an interrupt occurs, halt again

;--------------------------------------------------
; Data Section
;--------------------------------------------------
DriveId:        db 0
WelcomeMsg:     db 'NKOF Bootloader Initialized', 13, 10, 0
DiskExtMsg:     db 'BIOS disk extensions available', 13, 10, 0
NoDiskExtMsg:   db 'BIOS disk extensions not available', 13, 10, 0
HaltMsg:        db 'System halted', 13, 10, 0

;--------------------------------------------------
; Boot Sector Padding and Signature
;--------------------------------------------------
    times 510-($-$$) db 0   ; Pad with zeros to fill 510 bytes
    dw 0xAA55               ; Boot signature at end of boot sector