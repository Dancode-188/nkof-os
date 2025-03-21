;
; NKOF Stage 2 Bootloader
;
; This is loaded by the first stage bootloader and is responsible for:
; 1. Switching from real mode to protected mode
; 2. Setting up a more robust environment
; 3. Loading the kernel
; 4. Transferring control to the kernel

[BITS 16]                       ; Still in 16-bit Real Mode initially
[ORG 0x8000]                    ; Stage 2 will be loaded at this address

;--------------------------------------------------
; Stage 2 Entry Point
;--------------------------------------------------
stage2_start:
    ; Set up segment registers
    mov ax, 0                   ; Cannot directly move value to segment register
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00              ; Set stack below our code
    
    ; Print message to confirm stage 2 is loaded
    mov si, Stage2LoadedMsg
    call PrintString

    ; Get memory map using BIOS interrupt
    call GetMemoryMap
    
    ; Prepare for transition to protected mode
    call PrepareForProtectedMode
    
    ; Transition to protected mode
    call EnterProtectedMode
    
    ; We should never reach here in 16-bit mode
    mov si, ErrorMsg
    call PrintString
    jmp $                       ; Infinite loop

;--------------------------------------------------
; Function: PrintString (16-bit mode)
; Input: SI = Pointer to null-terminated string
;--------------------------------------------------
PrintString:
    push ax
    push bx
    
    mov ah, 0x0E                ; BIOS teletype function
    mov bx, 0x07                ; Page 0, text attribute 7 (light gray)
    
.loop:
    lodsb                       ; Load next character from SI into AL
    test al, al                 ; Check if character is 0 (end of string)
    jz .done
    
    int 0x10                    ; Print character
    jmp .loop
    
.done:
    pop bx
    pop ax
    ret

;--------------------------------------------------
; Function: GetMemoryMap
; Uses BIOS int 0x15, eax=0xE820 to get memory map
;--------------------------------------------------
GetMemoryMap:
    mov si, MemMapMsg
    call PrintString
    
    ; Set up buffer for memory map entries
    mov di, MemoryMapBuffer     ; Buffer to store entries
    xor ebx, ebx                ; EBX must be 0 for first call
    mov edx, 0x534D4150         ; 'SMAP' signature
    mov eax, 0xE820             ; Function code for get memory map
    mov ecx, 24                 ; Size of each entry (24 bytes)
    int 0x15                    ; BIOS interrupt
    
    ; Check if function is supported
    jc .error                   ; Carry flag set means error
    cmp eax, 0x534D4150         ; EAX should contain 'SMAP' on success
    jne .error
    
    ; Count and process entries (simplified for now)
    mov [MemoryMapEntries], byte 1  ; At least one entry if we get here
    
    ; Check if more entries
.next_entry:
    test ebx, ebx               ; EBX = 0 means last entry
    jz .done
    
    ; Get next entry
    add di, 24                  ; Point to next buffer position
    mov eax, 0xE820
    mov ecx, 24
    int 0x15
    jc .done                    ; If carry flag set, we're done
    
    ; Increment counter
    inc byte [MemoryMapEntries]
    jmp .next_entry
    
.error:
    mov si, MemMapErrorMsg
    call PrintString
    ret
    
.done:
    ; Print success message with entry count
    mov si, MemMapSuccessMsg
    call PrintString
    
    ; Convert entry count to ASCII and print
    mov al, [MemoryMapEntries]
    add al, '0'                 ; Convert to ASCII (works for single digit)
    mov ah, 0x0E
    mov bx, 0x07
    int 0x10
    
    ; Print newline
    mov al, 13                  ; Carriage return
    int 0x10
    mov al, 10                  ; Line feed
    int 0x10
    
    ret

;--------------------------------------------------
; Function: PrepareForProtectedMode
; Prepares system for transition to 32-bit protected mode
;--------------------------------------------------
PrepareForProtectedMode:
    mov si, PreparingProtectedMsg
    call PrintString
    
    ; Disable interrupts
    cli
    
    ; Load Global Descriptor Table
    lgdt [GDTDescriptor]
    
    ; Enable A20 line
    call EnableA20
    
    ret

;--------------------------------------------------
; Function: EnableA20
; Enables the A20 line for memory access above 1MB
;--------------------------------------------------
EnableA20:
    ; Method 1: BIOS function
    mov ax, 0x2401
    int 0x15
    jnc .done                   ; If successful, we're done
    
    ; Method 2: Keyboard controller
    call .wait_input
    mov al, 0xD1                ; Command: Write to output port
    out 0x64, al
    
    call .wait_input
    mov al, 0xDF                ; Enable A20 line
    out 0x60, al
    
    call .wait_input
    mov al, 0xFF                ; Pulse output port
    out 0x64, al
    
    call .wait_input
    
.done:
    ret
    
.wait_input:
    in al, 0x64
    test al, 2                  ; Check if input buffer is empty
    jnz .wait_input
    ret

;--------------------------------------------------
; Function: EnterProtectedMode
; Switches CPU from real mode to protected mode
;--------------------------------------------------
EnterProtectedMode:
    mov si, EnteringProtectedMsg
    call PrintString
    
    ; Set protected mode bit in CR0
    mov eax, cr0
    or eax, 1                   ; Set bit 0 (PE)
    mov cr0, eax
    
    ; Far jump to code segment to load CS with proper descriptor
    jmp 0x08:ProtectedModeEntry ; 0x08 is the code segment selector
    
;--------------------------------------------------
; 32-bit Protected Mode Code
;--------------------------------------------------
[BITS 32]                       ; Subsequent code is 32-bit

ProtectedModeEntry:
    ; Set up segment registers with data segment selector
    mov ax, 0x10                ; 0x10 is the data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Set up stack
    mov esp, 0x90000
    
    ; Clear screen by writing to video memory
    mov edi, 0xB8000            ; Video memory address
    mov ecx, 80*25              ; Screen size (80 columns x 25 rows)
    mov ax, 0x0720              ; Attribute (7) and space character (20h)
    rep stosw                   ; Repeat store word
    
    ; Display success message
    mov edi, 0xB8000
    mov esi, ProtectedModeMsg
    call Print32
    
    ; Halt the CPU
.halt:
    hlt
    jmp .halt

;--------------------------------------------------
; Function: Print32 (32-bit Protected Mode)
; Prints a string to video memory
; Input: ESI = String address, EDI = Video memory address
;--------------------------------------------------
Print32:
    push eax
    push edi
    
.loop:
    lodsb                       ; Load next character
    test al, al                 ; Check if end of string
    jz .done
    
    mov ah, 0x0F                ; White text on black background
    mov [edi], ax               ; Write to video memory
    add edi, 2                  ; Next character position
    jmp .loop
    
.done:
    pop edi
    pop eax
    ret

;--------------------------------------------------
; Global Descriptor Table (GDT)
;--------------------------------------------------
GDTStart:
    ; Null descriptor
    dq 0                        ; First entry must be null

    ; Code segment descriptor
    dw 0xFFFF                   ; Limit (bits 0-15)
    dw 0                        ; Base (bits 0-15)
    db 0                        ; Base (bits 16-23)
    db 10011010b                ; Access (present, ring 0, code segment, executable, direction 0, readable)
    db 11001111b                ; Granularity (4k pages, 32-bit protected mode) + Limit (bits 16-19)
    db 0                        ; Base (bits 24-31)

    ; Data segment descriptor
    dw 0xFFFF                   ; Limit (bits 0-15)
    dw 0                        ; Base (bits 0-15)
    db 0                        ; Base (bits 16-23)
    db 10010010b                ; Access (present, ring 0, data segment, direction 0, writable)
    db 11001111b                ; Granularity (4k pages, 32-bit protected mode) + Limit (bits 16-19)
    db 0                        ; Base (bits 24-31)
GDTEnd:

GDTDescriptor:
    dw GDTEnd - GDTStart - 1    ; Size of GDT (minus 1)
    dd GDTStart                 ; Address of GDT

;--------------------------------------------------
; Data Section
;--------------------------------------------------
Stage2LoadedMsg:      db 'Stage 2 bootloader loaded successfully', 13, 10, 0
MemMapMsg:            db 'Getting memory map...', 13, 10, 0
MemMapErrorMsg:       db 'Error getting memory map', 13, 10, 0
MemMapSuccessMsg:     db 'Memory map obtained. Entries: ', 0
PreparingProtectedMsg: db 'Preparing for protected mode...', 13, 10, 0
EnteringProtectedMsg: db 'Entering protected mode...', 13, 10, 0
ErrorMsg:             db 'Error: Should not reach here', 13, 10, 0
ProtectedModeMsg:     db 'Successfully entered 32-bit Protected Mode!', 0

; Variables
MemoryMapEntries:     db 0

; Buffer for memory map (24 bytes per entry, max 20 entries = 480 bytes)
MemoryMapBuffer:      times 480 db 0

; Padding to ensure the file is at least 512 bytes (for testing)
times 2048-($-$$) db 0
