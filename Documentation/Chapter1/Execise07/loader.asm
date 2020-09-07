    org 0x10000
    jmp label_start

%include "fat12.inc"

[SECTION gdt]
label_gdt: dd 0, 0
data32_desc: dd 0x0000FFFF, 0x00CF9200

gdt_len equ $ - label_gdt
gdt_ptr dw gdt_len - 1
        dd label_gdt

data32_sel equ data32_desc - label_gdt

[SECTION .s16]
[BITS 16]

label_start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    xor ax, ax
    mov ss, ax
    mov sp, 0x7C00 ; The bool sector isn't needed any more

    ; ------------------ Start loader ... ---------------------
    mov ax, 1301H
    mov bx, 000FH
    mov dx, 0200H
    mov cx, 12
    mov bp, start_loader_message
    int 10H

    ; ------------------ Open A20 address ---------------------
    push ax
    in al, 92H
    or al, 00000010b
    out 92H, al
    pop ax

    cli

    ; Setup CR0 and enable protected mode
    db 0x66
    lgdt [gdt_ptr]
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    mov ax, data32_sel
    mov fs, ax
    ; Disable protected mode. But this time, CPU is under
    ; big real mode that we can use FS register to reference
    ; address above 1 MB.
    mov eax, cr0
    and al, 11111110b
    mov cr0, eax

    sti
    jmp $

[SECTION .s16lib]
[BITS 16]

start_loader_message db "Start loader..."