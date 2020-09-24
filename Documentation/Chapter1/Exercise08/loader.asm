    org 0x10000
    jmp label_start

KERNEL_BASE equ 0x0
KERNEL_OFFSET equ 0x100000
TMP_KERNEL_BASE equ 0x0
TMP_KERNEL_OFFSET equ 0x7E00

%include "fat12.inc"

[SECTION gdt]
label_gdt: dd 0, 0
code32_desc: dd 0x0000FFFF, 0x00CF9A00
data32_desc: dd 0x0000FFFF, 0x00CF9200

gdt_len equ $ - label_gdt
gdt_ptr dw gdt_len - 1
        dd label_gdt

code32_sel equ code32_desc - label_gdt
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
    mov dx, 0100H
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
    
    ; ------------------ Reset floppy ---------------------
    xor ah, ah
    xor dl, dl
    int 13H

    ; ------------------ Search kernel.bin ---------------------
_search_in_root_directory:
    cmp word [unread_root_dir_sectors], 0
    jz _kernel_not_found

    dec word [unread_root_dir_sectors]

    ; Load a sector of root directory into memory for file searching
    xor ax, ax
    mov es, ax
    mov bx, 08000H
    mov ax, [root_dir_sector_offset]
    mov cl, 1
    call read_sectors

    ; Now, es:bx points to the root directory area
    mov si, kernel_name
    mov di, 08000H
    cld

    mov dx, 10H ; Number of root directory item per sector (512 / 32)

_search_kernel_bin:
    cmp dx, 0
    jz _go_to_next_sector_of_root_directory

    dec dx
    mov cx, 11

_compare_file_name:
    cmp cx, 0
    jz _kernel_found
    dec cx
    lodsb
    cmp al, byte [es:di]
    jz _continue_comparing
    jmp _go_to_next_entry

_continue_comparing:
    inc di
    jmp _compare_file_name

_go_to_next_entry:
    and di, 0FFE0H
    add di, 20H
    mov si, kernel_name
    jmp _search_kernel_bin

_go_to_next_sector_of_root_directory:
    inc word [root_dir_sector_offset]
    jmp _search_in_root_directory

_kernel_not_found:
    mov ax, 1301H
    mov bx, 008CH
    mov dx, 0200H
    mov cx, 35
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, kernel_not_found_prompt
    int 10H
    jmp $ ; Hang the system

_kernel_found:
    push es
    mov ax, 1301H
    mov bx, 000FH
    mov dx, 0200H
    mov cx, 22
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, kernel_found_prompt
    int 10H
    pop es

    mov ax, ROOT_DIR_SECTORS
    and di, 0FFE0H
    ; Make di points to the head address of current root directory entry
    add di, 01AH
    mov cx, [es:di]
    push cx ; Push the cluster index
    add cx, ax
    add cx, 17 ; 1 + 9 + 9 + CX - 2

    mov ax, TMP_KERNEL_BASE
    mov es, ax
    mov bx, TMP_KERNEL_OFFSET
    mov ax, cx

_continue_loading_kernel:
    push ax
    push bx
    mov ah, 0EH
    mov al, '*'
    mov bl, 0AH
    int 10H
    pop bx
    pop ax

    ; AX = sector index
    ; ES:BX = buffer address
    mov cl, 1
    call read_sectors
    pop ax ; The cluster index

    ; ------ Move kernel.bin from 0x7E00 to 0x100000, sector by sector ------
    push cx
    push ds
    push eax
    push esi
    push edi
    
    mov cx, 200H
    mov edi, dword [current_kernel_offset]

    mov ax, TMP_KERNEL_BASE
    mov ds, ax
    mov esi, TMP_KERNEL_OFFSET
_move_kernel:
    mov al, byte [ds:esi]
    mov byte [fs:edi], al
    inc esi
    inc edi
    loop _move_kernel

    mov ax, 0x1000 ; Loader is based on 0x10000
    mov ds, ax
    mov dword [current_kernel_offset], edi
    pop edi
    pop esi
    pop eax
    pop ds
    pop cx
    ; ------ End Move kernel.bin from 0x7E00 to 0x100000, sector by sector ------

    call get_fat_entry

    cmp ax, 0FFFH
    jz _finish_loading_kernel

    push ax
    add ax, ROOT_DIR_SECTORS
    add ax, 17
    add bx, [BPB_BytesPerSector]
    jmp _continue_loading_kernel

_finish_loading_kernel:
    mov ax, 1301H
    mov bx, 000FH
    mov dx, 0300H
    mov cx, 24
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, finish_loading_kernel_prompt
    int 10H

    jmp $

[SECTION .s16lib]
[BITS 16]
%include 's16lib.inc'

current_kernel_offset dd KERNEL_OFFSET

start_loader_message db "Start loader..."
unread_root_dir_sectors dw ROOT_DIR_SECTORS
root_dir_sector_offset  dw ROOT_DIR_SECTOR_INDEX
kernel_name             db 'KERNEL  BIN', 0
kernel_not_found_prompt db 'FatalError: Kernel cannot be found!'
kernel_found_prompt     db 'Kernel has been found '
finish_loading_kernel_prompt db 'Finish loading kernel :)'
