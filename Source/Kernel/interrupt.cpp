#include "io.h"
#include "interrupt.h"

extern "C" {
  #include "gate.h"
  #include "printk.h"
}

template<int NR> inline void build_irq() {
  PIC8259A::build_irq<NR>();
}

void (*interrupts[24])() {
  build_irq<0x20>,
  build_irq<0x21>,
  build_irq<0x22>,
  build_irq<0x23>,
  build_irq<0x24>,
  build_irq<0x25>,
  build_irq<0x26>,
  build_irq<0x27>,
  build_irq<0x28>,
  build_irq<0x29>,
  build_irq<0x2a>,
  build_irq<0x2b>,
  build_irq<0x2c>,
  build_irq<0x2d>,
  build_irq<0x2e>,
  build_irq<0x2f>,
  build_irq<0x30>,
  build_irq<0x31>,
  build_irq<0x32>,
  build_irq<0x33>,
  build_irq<0x34>,
  build_irq<0x35>,
  build_irq<0x36>,
  build_irq<0x37>
};

extern "C" void do_irq(pt_regs *regs, unsigned long nr) {
  PIC8259A::do_irq(regs, nr);
}

PIC8259A::PIC8259A() {
  printk(WHITE, BLACK, "Init 8259A...\n");

  for(int i = 32; i < 56; ++i) {
    set_intr_gate64(i, 2, reinterpret_cast<void *>(interrupts[i - 32]));
  }

  // 8259A master
  io::out8(0x20, 0x11);
  io::out8(0x21, 0x20);
  io::out8(0x21, 0x04);
  io::out8(0x21, 0x01);

  // 8259A slave
  io::out8(0xA0, 0x11);
  io::out8(0xA1, 0x28);
  io::out8(0xA1, 0x02);
  io::out8(0xA1, 0x01);

  // 8259A-M/S OCW1
  io::out8(0x21,0xfd);
  io::out8(0xa1,0xff);

  sti();

  printk(WHITE, BLACK, "Enable interrupt!\n");
}

template<int NR> inline void PIC8259A::build_irq() {
  __asm__ (
    "pushq $0x0 \n\t" // Interrupt doesn't have error code
    "cld \n\t"
    // We may replace this code later.
    // Now we share the restore code with exception handler.
    // The is the placeholder of handler address
    "pushq %%rax \n\t"
    // Save rax in stack
    "pushq %%rax \n\t"
    "movq %%es, %%rax \n\t"
    "pushq %%rax \n\t"
    "movq %%ds, %%rax \n\t"
    "pushq %%rax \n\t"
    "xorq %%rax, %%rax \n\t"
    "pushq %%rbp \n\t"
    "pushq %%rdi \n\t"
    "pushq %%rsi \n\t"
    "pushq %%rdx \n\t"
    "pushq %%rcx \n\t"
    "pushq %%rbx \n\t"
    "pushq %%r8 \n\t"
    "pushq %%r9 \n\t"
    "pushq %%r10 \n\t"
    "pushq %%r11 \n\t"
    "pushq %%r12 \n\t"
    "pushq %%r13 \n\t"
    "pushq %%r14 \n\t"
    "pushq %%r15 \n\t"
    // Load es and es with kernel data segment
    "movq $0x10, %%rdx \n\t"
    "movq %%rdx, %%ds \n\t"
    "movq %%rdx, %%es \n\t"
    // rdi points to current kernel stack 
    // which contains all registers info we saved before.
    "movq %%rsp, %%rdi \n\t"
    "leaq ret_from_intr(%%rip), %%rax \n\t"
    "pushq %%rax \n\t"
    "movq %0, %%rsi \n\t"
    "jmp do_irq \n\t"
    :
    :"i"(NR)
    :
  );
}

void PIC8259A::do_irq(pt_regs *regs, unsigned long nr) {
  unsigned char code = io::in8(0x60);
  printk(PURPLE, BLACK, "do_irq: %#2.2x\tkey code:%#8.8x\n", nr, code);


  io::out8(0x20, 0x20); // Send EOI to reset ISR register.
}

void PIC8259A::sti() {
  asm(
    "sti \n\t":::"memory"
  );
}

inline void PIC8259A::save_all() {
  __asm__ __volatile__(
    "cld \n\t"
    // We may replace this code later.
    // Now we share the restore code with exception handler.
    // The is the placeholder of handler address
    "pushq %%rax \n\t"
    "pushq %%rax \n\t"
    "movq %%es, %%rax \n\t"
    "pushq %%rax \n\t"
    "movq %%ds, %%rax \n\t"
    "pushq %%rax \n\t"
    "xorq %%rax, %rax \t\t"
    "pushq %rbp \n\t"
    "pushq %rdi \n\t"
    "pushq %rsi \n\t"
    "pushq %rdx \n\t"
    "pushq %rcx \n\t"
    "pushq %rbx \n\t"
    "pushq %r8 \n\t"
    "pushq %r9 \n\t"
    "pushq %r10 \n\t"
    "pushq %r11 \n\t"
    "pushq %r12 \n\t"
    "pushq %r13 \n\t"
    "pushq %r14 \n\t"
    "pushq %r15 \n\t"
    // Load es and es with kernel data segment
    "movq $0x10, %rdx \n\t"
    "movq %rdx, %%ds \n\t"
    "movq %rdx, %%es \n\t"
  );
}



