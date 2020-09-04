    org 0x7C00

jmp short label_start
nop

;------------- FAT12 data ---------------
BS_OEMName                  db 'YunaBoot'
BPB_BytesPerSector          dw 512
BPB_SectorsPerCluster       db 1
BPB_ResveredSectors         dw 1 ; The first sector is reserved
BPB_NumOfFATs               db 2 ; The number of FAT tables
BPB_RootDirectoryEntries    dw 224
BPB_TotalSectors            dw 2880
BPB_MediaDescriptor         db 0F0H
BPB_SectorsPerFAT           dw 9
BPB_SectorsPerTrack         dw 18
BPB_NumOfHeads              dw 2
BPB_HiddenSectors           dd 0
BPB_TotalSectors32          dd 0
BS_DrvNum                   db 0 ; Driver number of int 13H
BS_Reserved                 db 0
BS_BootSig                  db 029H
BS_VolID                    dd 0
BS_VolLabel                 db 'boot loader'
BS_FileSystem               db 'FAT12   '
;------------- End FAT12 data ---------------

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