#include "lib.h"

inline int strlen(char *string) {
  int __count;
  asm(
    "cld \n\t"
    "repne \n\t"
    "scasb \n\t"
    "notl %0 \n\t"
    "decl %0 \n\t"
    : "=c"(__count)
    : "D"(string), "a"(0), "0"(0xFFFFFFFF)
  );

  return __count;
}

inline void *memset(void *addr, unsigned char c, unsigned long count) {
  int d0, d1;
  // 0xAB * 0x0101 = 0xABAB
  // c * 0x0101010101010101UL means copying 8 bytes of c.
  unsigned long val = c * 0x0101010101010101UL;

  __asm__ __volatile__ (
    "rep \n\t"
    "stosq \n\t"
    "testb $4, %b3 \n\t" // memset the remains bytes
    "je 1f \n\t"
    "stosl \n\t"
    "1:\ttestb $2, %b3 \n\t"
    "je 2f \n\t"
    "stosw \n\t"
    "2:\ttestb $1, %b3 \n\t"
    "je 3f \n\t"
    "stosb \n\t"
    "3: \n\t"
    :"=&c"(d0), "=&D"(d1)
    :"a"(val),"q"(count),"0"(count/8),"1"(addr)
    :"memory"
  );

  return addr;
}
