extern "C" {
  #include "lib.h"
  #include "trap.h"
  #include "gate.h"
  #include "task.h"
  #include "memory.h"
  #include "printk.h"
}

#include "interrupt.h"

struct memory_descriptor gmd = {{0}, 0};

void pixel_fill32(unsigned int *fb, unsigned int rgb, int x0, int y0, int x1, int y1) {
  int x = 0, y = 0;
  rgb = rgb & 0x00FFFFFF;

  for (y = y0; y < y1; ++y) {
    for (x = x0; x < x1; ++x) {
      int offset = 1440 * y + x;
      fb[offset] = rgb;
    }
  }
}

extern "C" void Start_Kernel() {
  // pixel_fill32(fb, 0x00FF0000, 0, 0, 1440, 100);
  // putchar(fb, 1440, 720, 450, 0x0000FFFF, 0x00000000, 'Y');
  // char * str = _number(string_buffer, 1025, 16, 64, 32, SPECIAL);
  
  pos.scn_width = 1440;
  pos.scn_height = 900;
  pos.char_x = 0;
  pos.char_y = 0;
  pos.char_width = 8;
  pos.char_height = 16;
  pos.fb_addr = (unsigned int *)0xFFFF800000A00000;
  pos.fb_length = pos.scn_width * pos.scn_height * 4;

  printk(WHITE, BLACK, "[Info] %2.2d: %s", 1, "Hello, world.\n");

  tss_init();
  load_tr(8);

  vector_init();
  memory_init();
  // Divide 0 error
  // int j = 1;
  // --j;
  // int b = 3 / j;

  // __asm__ __volatile__ ("int3 \n\t");
  // int i = *(int *)0xFFFFF80000AA00000;

  PIC8259A pic;  
  while(1);
}