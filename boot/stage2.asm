[BITS 16]
[ORG 0x7000]

start:
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00

    mov [boot_drive], dl

    mov si, msg_e820
    call print_rm

    ; ---------------------------------------------------------------
    ; Get BIOS E820 memory map into physical address 0x8000.
    ; Layout: count at 0x8000, entries begin at 0x8004.
    ; ---------------------------------------------------------------
    mov word [0x8000], 0
    mov word [0x8002], 0

    xor ebx, ebx
    mov di, 0x8004

e820_next:
    cmp word [0x8000], 16
    jae e820_done

    ; Clear extended ACPI field in case BIOS returns 20 bytes.
    mov dword [es:di+20], 0

    mov eax, 0xE820
    mov edx, 0x534D4150       ; 'SMAP'
    mov ecx, 24
    int 0x15

    jc e820_error
    cmp eax, 0x534D4150
    jne e820_error

    inc word [0x8000]
    add di, 24

    test ebx, ebx
    jnz e820_next

e820_done:
    mov si, msg_e820_ok
    call print_rm

    ; ---------------------------------------------------------------
    ; Load kernel from sector 3 onward into physical 0x10000.
    ; ---------------------------------------------------------------
    mov si, msg_kernel
    call print_rm

    mov ax, 0x1000
    mov es, ax
    xor bx, bx

    mov ah, 0x02
    mov al, 64
    mov ch, 0
    mov cl, 3
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13

    jc disk_error

    mov si, msg_kernel_ok
    call print_rm

    ; ---------------------------------------------------------------
    ; Enter 32-bit protected mode.
    ; ---------------------------------------------------------------
    cli
    lgdt [gdt_descriptor]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp CODE_SEG:init_pm32

[BITS 32]

init_pm32:
    mov ax, DATA_SEG
    mov ds, ax
    mov ss, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov esp, 0x1F0000

    call 0x10000

.halt:
    hlt
    jmp .halt

[BITS 16]

e820_error:
    mov si, msg_e820_err
    call print_rm
    jmp $

disk_error:
    mov si, msg_disk_err
    call print_rm
    jmp $

print_rm:
    lodsb
    test al, al
    jz .done

    mov ah, 0x0E
    xor bh, bh
    int 0x10

    jmp print_rm

.done:
    ret

boot_drive db 0

msg_e820      db '  [STAGE2] Detecting memory (E820)...', 13, 10, 0
msg_e820_ok   db '  [STAGE2] E820 memory map OK', 13, 10, 0
msg_e820_err  db '  [STAGE2] E820 ERROR!', 13, 10, 0
msg_kernel    db '  [STAGE2] Loading kernel...', 13, 10, 0
msg_kernel_ok db '  [STAGE2] Kernel loaded OK', 13, 10, 0
msg_disk_err  db '  [STAGE2] DISK ERROR!', 13, 10, 0

; ---------------------------------------------------------------
; GDT
; ---------------------------------------------------------------

gdt_start:

gdt_null:
    dd 0x00000000
    dd 0x00000000

gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00

gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

times 510 - ($ - $$) db 0
dw 0xAA55
