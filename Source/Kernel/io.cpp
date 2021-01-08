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
