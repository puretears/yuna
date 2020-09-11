    org 0x7C00

BASE equ 07C00H
LOADER_BASE             equ 0x1000
LOADER_OFFSET           equ 0x0

    jmp short label_start
    nop

%include "fat12.inc"

label_start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BASE

    ; Clear screen
    ; AH = 06H Scroll screen in a specific range
    ; AL = 0 Clear screen
    ; BH = 07H 0 - black screen; 7H - white font;
    ; CH = start column number
    ; CL = start row number
    ; DH = end column number
    ; DL = end row number
    mov ax, 0600H
    mov bx, 0700H
    mov cx, 0
    mov dx, 0184FH ; 25 x 80 resolution
    int 10H

    ; Display on screen: Start booting ...
    ; AH = 13H
    ; AL = 01 move the cursor to the end of string
    ; BH = 0
    ; BL = 0FH Highlighted white color
    ; CX = lenth of the string
    ; DH = start row number
    ; DL = start column number
    ; ES:BP = address of the string
    mov ax, 1301H
    mov bx, 000FH
    mov cx, 0013H
    mov dx, 0000H
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, booting_message
    int 10H

    ; ========================== Search loader.bin
_search_in_root_directory:
    cmp word [unread_root_dir_sectors], 0
    jz _loader_not_found

    dec word [unread_root_dir_sectors]

    ; Load a sector of root directory into memory for file searching
    xor ax, ax
    mov es, ax
    mov bx, 08000H
    mov ax, [root_dir_sector_offset]
    mov cl, 1
    call read_sectors

    ; Now, es:bx points to the root directory area
    mov si, loader_name
    mov di, 08000H
    cld

    mov dx, 10H ; Number of root directory item per sector (512 / 32)

_search_loader_bin:
    cmp dx, 0
    jz _go_to_next_sector_of_root_directory

    dec dx
    mov cx, 11

_compare_file_name:
    cmp cx, 0
    jz _loader_found
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
    mov si, loader_name
    jmp _search_loader_bin

_go_to_next_sector_of_root_directory:
    inc word [root_dir_sector_offset]
    jmp _search_in_root_directory

_loader_not_found:
    mov ax, 1301H
    mov bx, 008CH
    mov dx, 0100H
    mov cx, 23
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, loader_not_found_prompt
    int 10H
    jmp $ ; Hang the system

_loader_found:
    mov ax, ROOT_DIR_SECTORS
    and di, 0FFE0H
    ; Make di points to the head address of current root directory entry
    add di, 01AH
    mov cx, [es:di]
    push cx ; Push the cluster index
    add cx, ax
    add cx, 17 ; 1 + 9 + 9 + CX - 2

    mov ax, LOADER_BASE
    mov es, ax
    mov bx, LOADER_OFFSET
    mov ax, cx

_continue_loading_loader:
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

    pop ax; AX = cluster number
    call get_fat_entry

    cmp ax, 0FFFH
    jz _finish_loading_loader

    push ax
    add ax, ROOT_DIR_SECTORS
    add ax, 17
    add bx, [BPB_BytesPerSector]
    jmp _continue_loading_loader

_finish_loading_loader:
    jmp LOADER_BASE:LOADER_OFFSET

%include 's16lib.inc'

booting_message         db 'Loading the loader '
unread_root_dir_sectors dw ROOT_DIR_SECTORS
root_dir_sector_offset  dw ROOT_DIR_SECTOR_INDEX
loader_not_found_prompt db 'ERROR: No loader found!'
loader_name             db 'LOADER  BIN', 0
times 510 - ($ - $$)    db 0
dw 0xAA55