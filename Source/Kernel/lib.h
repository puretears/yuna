#ifndef __LIB_H__
#define __LIB_H__

#define NULL 0

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

// inline int strlen(char *string) {
//   int i = 0;
//   while (*string++ != '\0') {
//     ++i;
//   }

//   return i;
// }

#endif
