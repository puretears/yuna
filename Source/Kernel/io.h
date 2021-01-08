#ifndef __IO_H__
#define __IO_H__

struct io {
  static void out8(unsigned short port, unsigned char val);
  static unsigned char in8(unsigned short port);
};

#endif
