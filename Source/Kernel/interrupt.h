#ifndef __INTERRUPT_H__
#define __INTERRUPT_H__

extern "C" {
  #include "ptrace.h"
}

class PIC8259A {
public:
  PIC8259A();
  template<int NR> static void build_irq();
  static void do_irq(pt_regs *regs, unsigned long nr);

private:
  void sti();
  void save_all();
};

#endif
