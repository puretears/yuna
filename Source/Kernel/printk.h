#ifndef __PRINTK_H__
#define __PRINTK_H__

#include "font.h"

extern unsigned char font_ascii[256][16];
char string_buffer[4096] = {0};

struct Position {
  int scn_width;
  int scn_height;

  int char_x;
  int char_y;
  int char_width;
  int char_height;

  unsigned int *fb_addr;    // Our frame buffer is 32-bit per unit
  unsigned long fb_length;
} pos;

#define RED 0x00FF0000
#define WHITE 0x00FFFFFF
#define BLACK 0x00000000
#define YELLOW 0x00FFFF00

#define LEFT    1
#define PLUS    1 << 1
#define SPACE   1 << 2
#define SPECIAL 1 << 3
#define LOWERCASE 1 << 4
#define SIGN 1 << 5

#define is_digit(c) ((c) >= '0' && (c) <= '9')
#define div(num, base) ({\
  int reminder; \
  asm("divq %%rcx" : "=a" (num), "=d" (reminder) : "0" (num), "1" (0), "c" (base)); \
  reminder; \
})

void putchar(unsigned int *fb, 
  int scn_width, 
  int x, int y, 
  int fg_color, int bg_color, unsigned char c);

int printk(unsigned int fg_color, unsigned int bg_color, const char *fmt, ...);

char *_number(char *str, long num, int base, int width, int precision, int type);
#endif
