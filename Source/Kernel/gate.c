#include "gate.h"

// inline void set_gate(unsigned long *selector, unsigned short attr, unsigned char ist, void *handler) {
//   unsigned long low, high;

//   asm volatile(
//     "movw %%dx, %%ax \n\t"
//     "andq $0x7, %%rcx \n\t"
//     "addq %4, %%rcx \n\t"
//     "shlq $32, %%rcx \n\t" // rcx - IST
//     "addq %%rcx, %%rax \n\t" // rax - IST + Selector
//     "xorq %%rcx, %%rcx \n\t"
//     "movl %%edx, %%ecx \n\t" // rcx - low 32-bit of handler address
//     "shrq $16, %%rcx \n\t"
//     "shlq $48, %%rcx \n\t" // rcx - offset 16...31 of handler hadress
//     "addq %%rcx, %%rax \n\t" // rax - IST + Selector + Offset 16...31
//     "movq %%rax, %0 \n\t"
//     "shrq $32, %%rdx \n\t"
//     "movq %%rdx, %1 \n\t"
//     :"=m"(*selector), "=m"(*(selector + 1)), "=&a"(low), "=&d"(high)
//     :"i"(attr << 8), "3"((unsigned long *)handler), "2"(0x8<<16), "c"(ist)
//     :"memory"
//   );
// }

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

volatile void set_tss64_desc(descriptor *desc) {
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

volatile void load_tr(unsigned char sel) {
  asm volatile(
    "ltr %%ax"
    :
    : "a"(sel << 3)
    : "memory"
  );
}

inline void set_intr_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0x8E, ist, handler);
}

inline void set_trap_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0x8F, ist, handler);
}

inline void set_user_intr_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0xEE, ist, handler);
}

inline void set_user_trap_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0xEF, ist, handler);
}

inline void set_tss64(
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