#include "linkage.h"

R15 =	0x00
R14 =	0x08
R13 =	0x10
R12 =	0x18
R11 =	0x20
R10 =	0x28
R9  =	0x30
R8  =	0x38
RBX =	0x40
RCX =	0x48
RDX =	0x50
RSI =	0x58
RDI =	0x60
RBP =	0x68
DS  =	0x70
ES  =	0x78
RAX =	0x80
FUNC    =	0x88
ERRCODE =	0x90
RIP =	0x98
CS  =	0xa0
RFLAGS = 0xa8
OLDRSP = 0xb0
OLDSS  = 0xb8

RESTORE_ALL:
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %r11
  popq %r10
  popq %r9
  popq %r8
  popq %rbx
  popq %rcx
  popq %rdx
  popq %rsi
  popq %rdi
  popq %rbp
  popq %rax
  movq %rax, %ds
  popq %rax
  movq %rax, %es
  popq %rax
  addq $0x10, %rsp // Jump FUNC and ERRORCODE
  iretq

ret_from_exception:
ENTRY(ret_from_intr)
  jmp RESTORE_ALL

ENTRY(divide_error)
  pushq $0 // Pesudo error code
  pushq %rax
  leaq do_divide_error(%rip), %rax
  xchgq %rax, (%rsp)

error_code:
  pushq %rax
  
  /*
   * Push segment registers are not supported in IA-32e mode.
   * We have to save them into general registers.
   */
  movq %es, %rax
  pushq %rax
  mov %ds, %rax
  pushq %rax

  xorq %rax, %rax

  pushq %rbp
  pushq %rdi
  pushq %rsi
  pushq %rdx
  pushq %rcx
  pushq %rbx
  pushq %r8
  pushq %r9
  pushq %r10
  pushq %r11
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15

  cld
  movq ERRCODE(%rsp), %rsi
  movq FUNC(%rsp), %rdx

  movq $0x10, %rdi
  movq %rdi, %ds
  movq %rdi, %es
  movq %rsp, %rdi
  callq *%rdx

  jmp ret_from_exception

ENTRY(debug)
  pushq $0
  pushq %rax
  leaq do_debug(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(nmi)
  pushq $0
  pushq %rax
  leaq do_nmi(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(int3)
  pushq $0
  pushq %rax
  leaq do_int3(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(overflow)
  pushq $0
  pushq %rax
  leaq do_overflow(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(bounds)
  pushq $0
  pushq %rax
  leaq do_bounds(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(invalid_opcode)
  pushq $0
  pushq %rax
  leaq do_invalid_opcode(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(device_unavailable)
  pushq $0
  pushq %rax
  leaq do_device_unavailable(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(double_fault)
  // We don't need to push $0, because double_fault
  // has a fixed error code 0.
  pushq %rax
  leaq do_double_fault(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(coprocessor_segment_overrun)
  push $0
  pushq %rax
  leaq do_coprocessor_segment_overrun(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(invalid_tss)
  // We don't need to push $0 because invalid_tss
  // has its own error code.
  pushq %rax
  leaq do_invalid_tss(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(segment_not_present)
  // We don't need to push $0 because segment_not_present
  // has its own error code.
  pushq %rax
  leaq do_segment_not_present(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(stack_segment_fault)
  // We don't need to push $0 because invalid_tss
  // has its own error code.
  pushq %rax
  leaq do_stack_segment_fault(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(general_protection)
  // We don't need to push $0 because general_protection
  // has its own error code.
  pushq %rax
  leaq do_general_protection(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(page_fault)
  // We don't need to push $0 because page_fault
  // has its own error code.
  pushq %rax
  leaq do_page_fault(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(x87_fpu_error)
  push $0
  pushq %rax
  leaq do_x87_fpu_error(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(alignment_check)
  // We don't need to push $0 because alignment_check
  // has its own error code.
  pushq %rax
  leaq do_alignment_check(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(machine_check)
  push $0
  pushq %rax
  leaq do_machine_check(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(simd_fp_exception)
  push $0
  pushq %rax
  leaq do_simd_fp_exception(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(virtualization_exception)
  push $0
  pushq %rax
  leaq do_virtualization_exception(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

ENTRY(control_protection_exception)
  // We don't need to push $0 because control_protection_exception
  // has its own error code.
  pushq %rax
  leaq do_control_protection_exception(%rip), %rax
  xchgq %rax, (%rsp)
  jmp error_code

// 22 - 31 are reserved by Intel
// 32 - 255 are user defined
