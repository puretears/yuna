#include "lib.h"
#include "trap.h"
#include "gate.h"
#include "printk.h"
#include "ptrace.h"

void do_divide_error(struct pt_regs *regs, unsigned long error_code) {
  printk(RED, BLACK, 
    "do_divice_error(0), ERROR_CODE:%#18lx,RSP:%#18lx,RIP:%#18lx",
    error_code, regs->rsp, regs->rip);
  while(1);
}

void do_debug(struct pt_regs *regs, unsigned long error_code) {
  printk(RED, BLACK,
    "do_debug(1), ERROR_CODE:%#18lx, RSP:%#18lx, RIP:%#18lx",
    error_code, regs->rsp, regs->rip);
  while(1);
}

void do_int3(struct pt_regs *regs, unsigned long error_code) {
  printk(RED, BLACK,
    "do_int3(3), ERROR_CODE:%#18lx, RSP:%#18lx, RIP:%#18lx",
    error_code, regs->rsp, regs->rip);
  while(1);
}

void vector_init() {
  set_trap_gate64(0, 1, divide_error);
  set_trap_gate64(1, 1, debug);

  set_user_trap_gate64(3, 1, int3);
}

void tss_init() {
  unsigned long stack = 0xFFFF800000007C00;
	set_tss64(stack, stack, stack, stack, stack, stack, stack, stack, stack, stack);

  set_tss64_desc(gdt + 8);
}
