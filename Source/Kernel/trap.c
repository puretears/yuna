#include "lib.h"
#include "trap.h"
#include "gate.h"
#include "printk.h"
#include "ptrace.h"

void exception_without_error_code(
  const char *name, struct pt_regs *regs, unsigned long error_code) {
  printk(RED, BLACK, 
    "%s, ERROR_CODE:%#18lx,RSP:%#18lx,RIP:%#18lx\n",
    name, error_code, regs->rsp, regs->rip);
  while(1);
}

void do_divide_error(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_divide_error(0)", regs, error_code);
}

void do_debug(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_debug(1)", regs, error_code);
}

void do_nmi(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_nmi(2)", regs, error_code);
}

void do_int3(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_int3(3)", regs, error_code);
}

void do_overflow(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_overflow(4)", regs, error_code);
}

void do_bounds(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_bounds(5)", regs, error_code);
}

void do_invalid_opcode(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_invalid_opcode(6)", regs, error_code);
}

void do_device_unavailable(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_device_unavailable(7)", regs, error_code);
}

void do_double_fault(struct pt_regs *regs, unsigned long error_code) {
  printk(RED, BLACK,
    "do_double_fault(8), ERROR_CODE:%#18lx, RSP:%#18lx, RIP:%#18lx\n",
    error_code, regs->rsp, regs->rip);
  while(1);
}

void do_coprocessor_segment_overrun(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_coprocessor_segment_overrun(9)", regs, error_code);
}

/**
 * A general error code
 *  31                       16   15                              2      1       0    
 * ┌───────────────────────────┬─────────────────────────────┬──────┬───────┬───────┐
 * │         Reserved          │    Segment Selector Index   │  TI  │ Index │  EXT  │
 * └───────────────────────────┴─────────────────────────────┴──────┴───────┴───────┘
 * 
 * More information: Intel SDM vol3 - 6.13
 */
void exception_with_general_error_code(const char *name, struct pt_regs *regs, unsigned long error_code) {
  printk(RED, BLACK,
    "%s, ERROR_CODE:%#18lx, RSP:%#18lx, RIP:%#18lx\n",
    name, error_code, regs->rsp, regs->rip);
  printk(RED, BLACK, "Segment Selector Index: %#10x\n", error_code & 0xFFF8);

  const char *msg = "The exception occurred during delivery of an event external \
  to the program, such as an interrupt or an earlier exception.";

  if (error_code & 1) {
    printk(RED, BLACK, msg);
  }
  
  if (error_code & 2) {
    msg = "The index portion of the error code refers to a gate descriptor in the IDT.";
    printk(RED, BLACK, msg);
  }
  else {
    if (error_code & 4) {
      msg = "The index portion of the error code refers to a descriptor in the LDT.";
    }
    else {
      msg = "The index portion of the error code refers to a descriptor in the current GDT.";
    }

    printk(RED, BLACK, msg);
  }

  while(1);
}

void do_invalid_tss(struct pt_regs *regs, unsigned long error_code) {
  exception_with_general_error_code("do_invalid_tss(10)", regs, error_code);
}

void do_segment_not_present(struct pt_regs *regs, unsigned long error_code) {
  exception_with_general_error_code("do_segment_not_present(11)", regs, error_code);
}

void do_stack_segment_fault(struct pt_regs *regs, unsigned long error_code) {
  exception_with_general_error_code("do_stack_segment_fault(12)", regs, error_code);
}

void do_general_protection(struct pt_regs *regs, unsigned long error_code) {
  exception_with_general_error_code("do_general_protection(13)", regs, error_code);
}

/**
 * #PF has its own error code format.
 * Refer to Instel SDM vol3 6.15 for more information.
 * 
 * TODO: PK(bit 5), SS(bit 6) and SGX(bit 15) are not handled yet.
*/
void do_page_fault(struct pt_regs *regs, unsigned long error_code) {
  unsigned long cr2 = 0;

  // DONOT reorder the assignment from %%cr2 to cr2.
  // The processor loads the CR2 register with the inear address 
  // that generated the exception. We can use this address to locate the
  // page directory and page item.
  __asm__ __volatile__("movq %%cr2, %0":"=r"(cr2)::"memory");
  printk(RED,BLACK,
    "do_page_fault(14), ERROR_CODE:%#018lx, RSP:%#018lx, RIP:%#018lx\n",
    error_code , regs->rsp , regs->rip);

   if (error_code & 1) {
     printk(RED, BLACK, "The fault was caused by a page-level protection violation.\t");
   }
   else  {
     printk(RED, BLACK, "The fault was caused by a non-present page.\t");
   }

   if (error_code & 2) {
     printk(RED, BLACK, "The access causing the fault was a write.\t");
   }
   else {
     printk(RED, BLACK, "The access causing the fault was a read.\t");
   }

   if (error_code & 4) {
     printk(RED, BLACK, "A user mode access caused the fault.\t");
   }
   else {
     printk(RED, BLACK, "A supervisor mode (Ring0/1/2) access caused the fault.\t");
   }

   if (error_code & 8) {
     printk(RED, BLACK, "The fault was caused by a reserved bit set to 1 in some paging-structure entry.\t");
   }
   
   if (error_code & 0x10) {
     printk(RED, BLACK, "The fault was caused by an instruction fetch.\t");
   }

   printk(RED, BLACK, "CR2:%#18lx\n", cr2);

   while(1);
}

void do_x87_fpu_error(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_x87_fpu_error(16)", regs, error_code);
}

void do_alignment_check(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_alignment_check(17)", regs, error_code);
}

void do_machine_check(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_alignment_check(18)", regs, error_code);
}

void do_simd_fp_exception(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_simd_fp_exception(19)", regs, error_code);
}

void do_virtualization_exception(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_virtualization_exception(20)", regs, error_code);
}

void do_control_protection_exception(struct pt_regs *regs, unsigned long error_code) {
  exception_without_error_code("do_control_protection_exception(21)", regs, error_code);
}

void vector_init() {
  set_trap_gate64(0, 1, divide_error);
  set_trap_gate64(1, 1, debug);
  set_intr_gate64(2, 1, nmi);

  set_user_trap_gate64(3, 1, int3);
  set_user_trap_gate64(4, 1, overflow);
  set_user_trap_gate64(4, 1, bounds);

  set_trap_gate64(6, 1, invalid_opcode);
  set_trap_gate64(7, 1, device_unavailable);
  set_trap_gate64(8, 1, double_fault);
  set_trap_gate64(9, 1, coprocessor_segment_overrun);
  set_trap_gate64(10, 1, invalid_tss);
  set_trap_gate64(11, 1, segment_not_present);
  set_trap_gate64(12, 1, stack_segment_fault);
  set_trap_gate64(13, 1, general_protection);
  set_trap_gate64(14, 1, page_fault);
  /* 15 is reserved by Intel.*/
  set_trap_gate64(16, 1, x87_fpu_error);
  set_trap_gate64(17, 1, alignment_check);
  set_trap_gate64(18, 1, simd_fp_exception);
  set_trap_gate64(19, 1, virtualization_exception);
  set_trap_gate64(20, 1, control_protection_exception);
}

void tss_init() {
  unsigned long stack = 0xFFFF800000007C00;
	set_tss64(stack, stack, stack, stack, stack, stack, stack, stack, stack, stack);

  set_tss64_desc(gdt + 8);
}
