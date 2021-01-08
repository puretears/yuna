#include "io.h"

void io::out8(unsigned short port, unsigned char val) {
  __asm__ __volatile__(
    "outb %0, %%dx \n\t"
    "mfence \n\t"
    :
    :"a"(val), "d"(port)
    :"memory"
  );
}

unsigned char io::in8(unsigned short port) {
  unsigned char val = 0;

  __asm__ __volatile__(
    "inb %%dx, %0 \n\t"
    "mfence \n\t"
    :"=a"(val)
    :"d"(port)
    :"memory"
  );

  return val;
}
