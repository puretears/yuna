.section .text

.global _start // Help linker recognizes this symbol

_start:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %ss
    mov $0x7E00, %esp

    lgdt gdt_ptr(%rip)
    lidt idt_ptr(%rip)

    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %ss
    mov $0x7E00, %rsp

    movq $0x101000, %rax
    movq %rax, %cr3

    movq _kernel_64bit_code(%rip), %rax
    pushq $0x08
    pushq %rax
    lretq // Simulate jmp 0x08:_kernel_64bit_code

    // kernel 64-bit code
_kernel_64bit_code:
    .quad _entry64
_entry64:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %ss
    movq $0xFFFF800000007E00, %rsp

    movq _jump_to_kernel(%rip), %rax
    pushq $0x08
    pushq %rax
    lretq

_jump_to_kernel:
    .quad Start_Kernel

 
.align 8

.org 0x1000 // 0x101000
_PML4:
    .quad 0x102003 // 0x00000000, 00000000
    .fill 255, 8, 0
    .quad 0x102003 // 0xFFFF8000, 00000000
    .fill 255, 8, 0

.org 0x2000
_PDPTR:
    .quad 0x103003
    .fill 511, 0, 0

.org 0x3000
_PDE:
    .quad 0x000083
    .quad 0x200083
    .quad 0x400083
    .quad 0x600083
    .quad 0x800083
    .quad 0xe0000083 // Frame buffer FFFF800000A00000
    .quad 0xe0200083
    .quad 0xe0400083
    .quad 0xe0600083
    .quad 0xe0800083
    .quad 0xe0a00083
    .quad 0xe0c00083
    .quad 0xe0e00083
    .fill 499, 8, 0

.align 8

.section .data
// GDT
.global gdt
gdt:
    .quad 0x0000000000000000 /* 0 - NULL descriptor */
    .quad 0x0020980000000000 /* 1 - Kernel 64-bit code segment */
    .quad 0x0000920000000000 /* 2 - Kernel 64-bit data segment */
    .quad 0x0020F80000000000 /* 3 - User 64-bit code segment */
    .quad 0x0000F20000000000 /* 4 - User 64-bit data segment */
    .quad 0x0000000000000000 /* 5 - Reserved for future use */
    .quad 0x0000000000000000 /* 6 - Reserved for future use */
    .quad 0x0000000000000000 /* 7 - Reserved for future use */
    .fill 10, 8, 0           /* 8,9 - TSS64 */
gdt_end:

gdt_ptr:
    .word gdt_end - gdt - 1
    .quad gdt

// IDT
.global idt
idt:
    .fill 512, 8, 0 // 256 x 64
idt_end:

idt_ptr:
    .word idt_end - idt - 1
    .quad idt

// TSS
.global tss
tss:
    .fill 13, 8, 0
tss_end:
