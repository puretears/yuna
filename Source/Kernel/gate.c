#include "gate.h"

volatile void set_gate64(gate_descriptor *desc, unsigned char attr, unsigned char ist, void *handler) {
  unsigned long high = 0;
  unsigned long low = 0;

  unsigned short offset15_to_0 = (unsigned long)handler & 0xFFFF;
  unsigned short offset31_to_16 = ((unsigned long)handler >> 16) & 0xFFFF;

  low += offset15_to_0;
  low += 0x8 << 16;
  ist &= 0x7;
  low += ((unsigned long)ist) << 32;
  low += ((unsigned long)attr) << 8 << 32;
  low += ((unsigned long)offset31_to_16) << 48;

  high += ((unsigned long)handler >> 32) & 0xFFFFFFFF;

  desc->low = low;
  desc->high = high;
}

void set_tss64_desc(descriptor *desc) {
  unsigned long high = 0;
  unsigned long low = 0;

  unsigned short limit = 103;
  unsigned short base15_to_0 = ((unsigned long)tss) & 0xFFFF;
  unsigned char base23_to_16 = (((unsigned long)tss) >> 16) & 0xFF;
  unsigned char attr = 0x89; // Present available TSS64
  unsigned char base31_to_24 = (((unsigned long)tss) >> 24) & 0xFF;

  desc->low = (base15_to_0 << 16) + limit;
  desc->high = (base31_to_24 << 24) + (attr << 8) + base23_to_16;

  ++desc;

  desc->low = ((unsigned long)tss) >> 32 & 0xFFFFFFFF;
  desc->high = 0;
}

void load_tr(unsigned char sel) {
  asm volatile(
    "ltr %%ax"
    :
    : "a"(sel << 3)
    : "memory"
  );
}

void set_intr_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0x8E, ist, handler);
}

void set_trap_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0x8F, ist, handler);
}

void set_user_intr_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0xEE, ist, handler);
}

void set_user_trap_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0xEF, ist, handler);
}

void set_tss64(
  unsigned long rsp0, unsigned long rsp1, unsigned long rsp2,
  unsigned long ist1, unsigned long ist2, unsigned long ist3,
  unsigned long ist4, unsigned long ist5, unsigned long ist6,
  unsigned long ist7) {
  *(unsigned long *)(tss + 1) = rsp0;
  *(unsigned long *)(tss + 3) = rsp1;
  *(unsigned long *)(tss + 5) = rsp2;

  *(unsigned long *)(tss + 9) = ist1;
  *(unsigned long *)(tss + 11) = ist2;
  *(unsigned long *)(tss + 13) = ist3;
  *(unsigned long *)(tss + 15) = ist4;
  *(unsigned long *)(tss + 17) = ist5;
  *(unsigned long *)(tss + 19) = ist6;
  *(unsigned long *)(tss + 21) = ist7;
}