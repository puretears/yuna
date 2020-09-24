    ; al - The digit to be displayed
disp_al:
    push gs
    push edi
    push ecx
    push edx

    mov cx, 0B800H
    mov gs, cx

    mov edi, [global_disp_position]
    mov ah, 0FH
    mov dl, al
    shr al, 4
    mov cx, 2

_begin:
    and al, 0FH
    cmp al, 9
    ja _1
    add al, '0' ; convert number to ascii
    jmp _2
_1:
    sub al, 0AH
    add al, 'A'
_2:
    mov [gs:edi], ax
    add edi, 2

    mov al, dl
    loop _begin

    mov [global_disp_position], edi

    pop edx
    pop ecx
    pop edi
    pop gs
    ret

    ; al - The ascii to be displayed
disp_ascii:
    push gs
    push cx
    push edi
    
    mov cx, 0B800H
    mov gs, cx

    mov edi, [global_disp_position]
    mov ah, 0FH
    mov [gs:edi], ax
    add edi, 2

    mov [global_disp_position], edi

    pop edi
    pop cx
    pop gs
    ret

    ; ax - address of string
disp_string:
    push cx
    push gs
    push si

    mov cx, 0B800H
    mov gs, cx
    mov si, ax

_continue_displaying_string:
    lodsb
    cmp al, 0
    jz _disp_string_end
    call disp_ascii
    jmp _continue_displaying_string
_disp_string_end:

    pop si
    pop gs
    pop cx
    ret

    ; Switch to the next line
new_line:
    ; (global_disp_position / 160 + 1) * 160
    push eax
    push ebx
    push edx
    xor edx, edx
    mov eax, [global_disp_position]
    mov ebx, 160
    div ebx
    inc eax
    xor edx, edx
    mul ebx
    mov [global_disp_position], eax
    pop edx
    pop ebx
    pop eax
    ret

    ; Clean up the screen
clear_screen:
    push cx
    mov dword [global_disp_position], 0
    mov cx, 2000
_clear:
    mov al, ' '
    call disp_ascii
    loop _clear
    mov dword [global_disp_position], 0
    pop cx
    ret


global_disp_position dd 160
