    org 0x10000
    jmp label_start

KERNEL_BASE equ 0x0
KERNEL_OFFSET equ 0x100000
TMP_KERNEL_BASE equ 0x0
TMP_KERNEL_OFFSET equ 0x7E00
MEMORY_INFO_BUFFER equ 0x7E00

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

[SECTION gdt64]
label_gdt64: dd 0, 0
code64_desc: dd 0x00000000, 0x00209A00
data64_desc: dd 0x00000000, 0x00009200

gdt64_len equ $ - label_gdt64
gdt64_ptr dw gdt64_len - 1
          dd label_gdt64

code64_sel equ code64_desc - label_gdt64
data64_sel equ data64_desc - label_gdt64

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
    mov ax, start_loader_message
    call disp_string
    call new_line
    ; mov ax, 1301H
    ; mov bx, 000FH
    ; mov dx, 0100H
    ; mov cx, 12
    ; mov bp, start_loader_message
    ; int 10H

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
    mov ax, kernel_not_found_prompt
    call disp_string
    call new_line
    jmp $ ; Hang the system

_kernel_found:
    push es
    mov ax, kernel_found_prompt
    call disp_string
    call new_line
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
    xor ax, ax
    mov al, '*'
    call disp_ascii
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
    ; add bx, [BPB_BytesPerSector]
    jmp _continue_loading_kernel

_finish_loading_kernel:
    mov ax, finish_loading_kernel_prompt
    call disp_string
    call new_line
    ; mov ax, 1301H
    ; mov bx, 000FH
    ; mov dx, 0300H
    ; mov cx, 24
    ; push ax
    ; mov ax, ds
    ; mov es, ax
    ; pop ax
    ; mov bp, finish_loading_kernel_prompt
    ; int 10H

    ; Kill motor
    push dx
    mov dx, 03F2H
    mov al, 0
    out dx, al
    pop dx

    ; ------------------ Get memory info ---------------------
    mov ax, memory_info_prompt
    call disp_string
    call new_line

    mov ebx, 0
    mov ax, 0
    mov es, ax
    mov di, MEMORY_INFO_BUFFER

_load_mem_info:
    mov eax, 0E820H
    mov ecx, 20
    mov edx, 534D4150H
    int 15H
    jc _load_mem_info_failed
    add di, 20
    cmp ebx, 0
    jne _load_mem_info
    jmp _load_mem_info_successed

_load_mem_info_failed:
    mov ax, load_mem_info_failed_message
    call disp_string
    call new_line
    jmp $

_load_mem_info_successed:
    mov ax, load_mem_info_successed_message
    call disp_string
    call new_line

    ; Get SVGA info
    mov ax, vbe_info_prompt
    call disp_string
    call new_line

    ; Write VBE controller info into es:di
    xor ax, ax
    mov es, ax
    mov di, 8000H
    mov ax, 4F00H
    int 10H

    cmp ax, 004FH
    jz _get_svga_vbe_info_successed

    mov ax, get_svga_vbe_info_failed
    call disp_string
    call new_line
    jmp $

_get_svga_vbe_info_successed:
    mov ax, get_svga_vbe_info_successed
    call disp_string
    call new_line

    ; Get SVGA mode info
    mov ax, svga_mode_prompt
    call disp_string
    call new_line

    xor ax, ax
    mov es, ax
    mov si, 800EH
    mov esi, dword [es:si]
    mov edi, 8200H

_vga_mode_list:
    mov cx, word [es:esi]

    push ax
    xor ax, ax
    mov al, ch
    call disp_al

    xor ax, ax
    mov al, cl
    call disp_al

    xor ax, ax
    mov al, ' '
    call disp_ascii
    pop ax

    cmp cx, 0FFFFH
    jz _finish_loading_svga_mode

    mov ax, 4F01H
    int 10h
    cmp ax, 004FH
    jnz _load_svga_mode_failed

    add esi, 2
    add edi, 0x100
    jmp _vga_mode_list

_load_svga_mode_failed:
    mov ax, load_svga_mode_failed
    call disp_string
    call new_line
    jmp $

_finish_loading_svga_mode:
    mov ax, finish_loading_svga_mode
    call disp_string
    call new_line

    ;Set SVGA mode
    ;mov ax, 4F02H
    ;mov bx, 4180H ; 1440 * 900
    ;int 10H

    ;cmp ax, 004FH
    ;jz _set_svga_mode_successed
    jmp _set_svga_mode_successed

    mov ax, set_svga_mode_failed
    call disp_string
    call new_line
    jmp $

_set_svga_mode_successed:
    cli
    
    db 0x66
    lgdt [gdt_ptr]

    db 0x66
    lidt [idt_ptr]

    mov eax, cr0
    or eax, 1
    mov cr0, eax

    jmp dword code32_sel:pm_mode_start

[SECTION .s32]
[BITS 32]
pm_mode_start:
    mov ax, data32_sel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov esp, 7E00H

    call intel64_available
    test eax, eax
    jnz _intel64_available

    ; We cannot display anything after changing the display mode.
    ; Just hang up the system
    jmp $

_intel64_available:
    ; -------------- Temporary Page Table --------------
    mov dword [0x90000], 0x91007  ; PML4
    mov dword [0x90800], 0x91007  ; PML4

    mov dword [0x91000], 0x92007  ; PDPT

    mov dword [0x92000], 0x000083 ; PD
    mov dword [0x92008], 0x200083
    mov dword [0x92010], 0x400083
    mov dword [0x92018], 0x600083
    mov dword [0x92020], 0x800083
    mov dword [0x92028], 0xa00083
    ; ------------- End Temporary Page Table ------------

    lgdt [gdt64_ptr]

    mov ax, 10H
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 7E00H

    ; Open PAE
    mov eax, cr4
    bts eax, 5
    mov cr4, eax

    ; Load CR3
    mov eax, 90000H
    mov cr3, eax

    ; Enable long mode
    mov ecx, 0C0000080H
    rdmsr
    bts eax, 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    bts eax, 0
    bts eax, 31
    mov cr0, eax

    jmp code64_sel:KERNEL_OFFSET

    ; Return
    ; eax: 1 - available; 0 - unavailable;
intel64_available:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    setnb al
    jb _long_mode_done
    mov eax, 0x80000001
    cpuid
    bt edx, 29
    setc al
_long_mode_done:
    movzx eax, al
    ret

[SECTION .s16lib]
[BITS 16]
%include 's16lib.inc'
%include 'display16.inc'

current_kernel_offset dd KERNEL_OFFSET

start_loader_message db "Start loader...", 0
unread_root_dir_sectors dw ROOT_DIR_SECTORS
root_dir_sector_offset  dw ROOT_DIR_SECTOR_INDEX
kernel_name             db 'KERNEL  BIN', 0
kernel_not_found_prompt db 'FatalError: Kernel cannot be found!', 0
kernel_found_prompt     db 'Kernel has been found ', 0
finish_loading_kernel_prompt db 'Finish loading kernel :)', 0

memory_info_prompt db '====== Memory Info ======', 0
load_mem_info_failed_message db 'Load memory info failed.', 0
load_mem_info_successed_message db 'Load memory info successed.', 0

vbe_info_prompt db '====== VBE Info ======', 0
get_svga_vbe_info_failed db 'Get SVGA VBE info failed.', 0
get_svga_vbe_info_successed db 'Get SVGA VBE info successed.', 0

svga_mode_prompt db '====== SVGA mode ======', 0
load_svga_mode_failed db 'Get SVGA mode failed.', 0
finish_loading_svga_mode db 'Finish loading SVGA mode.', 0
set_svga_mode_failed db 'Set SVGA mode failed.', 0

; ========= Tmp IDE =========
; Because interrupts are disabled during setting up process,
; these dummy IDT entries are enough.
idt:
    times 0x50 dq 0
idt_end:

idt_ptr:
    dw idt_end - idt - 1
    dd idt
