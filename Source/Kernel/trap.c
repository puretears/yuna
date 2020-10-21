#include "lib.h"
#include "trap.h"
#include "gate.h"
#include "printk.h"

void do_divide_error(unsigned long rsp, unsigned long error_code) {
  unsigned long *p = (unsigned long *)(rsp + 0x98);
  printk(RED, BLACK, "do_divice_error, ERROR_CODE:%#18lx,RSP:%#18lx,RIP:%#18lx", error_code, rsp, *p);
  while(1);
}

void vector_init() {
  set_trap_gate64(0, 1, divide_error);
}

void tss_init() {
  unsigned long stack = 0xFFFF800000007C00;
	set_tss64(stack, stack, stack, stack, stack, stack, stack, stack, stack, stack);

  set_tss64_desc(gdt + 8);
}
