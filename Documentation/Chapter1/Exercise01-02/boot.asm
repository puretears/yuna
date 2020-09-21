    org 0x7C00
BASE equ 07C00H

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
    mov cx, 0014H
    mov dx, 0000H
    push ax
    mov ax, ds
    mov es, ax
    pop ax
    mov bp, booting_message
    int 10H

    ; Reset floppy
    ; DL = 00H floopy disk
    ; DL = 80H hard disk
    xor ah, ah
    xor dl, dl
    int 13H
    
    ; Hanging the system
    jmp $

booting_message     db 'Hello, Kernel World!'
times 510 - ($ - $$) db 0
dw 0xAA55