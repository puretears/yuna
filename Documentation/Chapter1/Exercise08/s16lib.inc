; ========================== Helpers
    ; AX - The starting sector number
    ; CL - How many sectors will be read
    ; ES:BX - The buffer address
read_sectors:
    push bp
    mov bp, sp
    ; The following two instructs are a tip that save cl in a word
    sub sp, 2
    mov [bp - 2], cl 
    push dx
    push bx

    mov bl, [BPB_SectorsPerTrack]
    div bl

    inc ah ; ah - reminder, ah + 1 - the starting sector number in a track
    mov cl, ah

    mov dh, al ; al - quotient
    and dh, 1  ; dh - disk header number
    mov dl, [BS_DrvNum]

    shr al, 1
    mov ch, al ; ch - cylinder number

    pop bx ; bx - bufer address
_continue_reading:
    mov ah, 2
    mov al, byte [bp - 2]
    int 13H
    jc _continue_reading

    pop dx
    add sp, 2
    pop bp
    ret

    ; AX - FAT index 
get_fat_entry:
    push bp
    mov bp, sp
    sub sp, 2 ; A temporary variable which can be used as [bp - 2]
    push es
    push bx

    push ax
    xor ax, ax
    mov es, ax
    pop ax

    mov word [bp - 2], 0
    mov bx, 3
    mul bx
    mov bx, 2
    div bx

    cmp dx, 0 ; DX - reminder
    jz _even_index
    mov word [bp - 2], 1
_even_index:
    xor dx, dx
    mov bx, [BPB_BytesPerSector]
    div bx

    add ax, FAT1_SECTOR_INDEX
    mov bx, 8000H
    mov cl, 2
    call read_sectors

    add bx, dx
    mov ax, [es:bx] ; The FAT entry
    cmp word [bp - 2], 1
    jnz _even_index1
    shr ax, 4
_even_index1:
    and ax, 0FFFH

    ; Restore the stack and return
    pop bx
    pop es
    add sp, 2
    pop bp
    ret