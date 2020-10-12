#include <stdarg.h>
#include "lib.h"
#include "printk.h"

void _new_line();
void _backspace(int fg_color, int bg_color);
void _tab(int fg_color, int bg_color);
void _ascii(unsigned char c, int fg_color, int bg_color);
void _adjust_position();

int _atoi(const char **);
int _parse_flags(const char **);
int _parse_width(const char **);
int _parse_precision(const char **);
int _parse_sub_specifier(const char **);
char *_number(char *str, long num, int base, int width, int precision, int type);

void putchar(unsigned int *fb, 
  int scn_width, 
  int x, int y, 
  int fg_color, int bg_color, unsigned char c) {
  int row_per_char = 0, column_per_char = 0;
  int fg_test = 0;
  unsigned int *fb_addr = NULL;
  unsigned char *font_pixel = font_ascii[c];

  for (; row_per_char < 16; ++row_per_char) {
    fb_addr = fb + scn_width * (y + row_per_char) + x;
    fg_test = 0x100;

    for (column_per_char = 0; column_per_char < 8; ++column_per_char) {
      fg_test = fg_test >> 1;

      if (*font_pixel & fg_test) {
        *fb_addr = fg_color;
      }
      else {
        *fb_addr = bg_color;
      }

      ++fb_addr;
    }

    ++font_pixel;
  }
}

int vsprintf(char *buf, const char *fmt, va_list args) {
  char *str;
  int flags;
  int width;
  int precision;
  int sub_specifier;

  for (str = buf; *fmt; ++fmt) {
    // Normal characters
    if (*fmt != '%') {
      *str = *fmt;
      ++str;
      continue;
    }

    // %[flags][width][.precision][length]specifier
    // Parse flags
    ++fmt;
    flags = _parse_flags(&fmt);

    // Parse width
    width = _parse_width(&fmt);

    // Parse precision
    precision = _parse_precision(&fmt);

    // Parse sub specifier
    sub_specifier = _parse_sub_specifier(&fmt);

    // Parse specifier
    char *s;
    int str_len, i;
    switch (*fmt) {
      case 'c': // Character
        if (!(flags & LEFT)) {
          while (--width > 0) { *str++ = ' '; }
        }

        *str++ = (unsigned char)va_arg(args, int);

        while (--width > 0) { *str++ = ' '; }
        break;
      case 's': // String
        s = va_arg(args, char *);
        if (!s) { s = '\0'; }
        str_len = strlen(s);

        if (precision < 0) {
          precision = str_len;
        }
        else if (str_len > precision) {
          str_len = precision;
        }

        if (!(flags & LEFT)) {
          while (str_len < width--) {
            *str++ = ' ';
          }
        }

        for (i = 0; i < str_len; ++i) {
          *str++ = *s++;
        }

        while (str_len < width--) {
          *str++ = ' ';
        }
        break;
      case 'o': // Unsigned octal
        if (sub_specifier == 'l') {
          str = _number(str, va_arg(args, unsigned long), 8, width, precision, flags);
        }
        else {
          str = _number(str, va_arg(args, unsigned int), 8, width, precision, flags);
        }

        break;
      case 'p': // Pointer address
        if (width == -1) {
          width = 2 * sizeof(void *);
        }

        str = _number(str, (unsigned long)va_arg(args, void *), 16, width, precision, flags);
        break;
      case 'x':
        flags |= LOWERCASE;
      case 'X': // Hexidecimal
        if (sub_specifier == 'l') {
          str = _number(str, va_arg(args, unsigned long), 16, width, precision, flags);
        }
        else {
          str = _number(str, va_arg(args, unsigned int), 16, width, precision, flags);
        }
        break;
      case 'd':
      case 'i': // Signed decimal integer
        flags |= SIGN;
        if (sub_specifier == 'l') {
          str = _number(str, va_arg(args, long), 10, width, precision, flags);
        }
        else {
          str = _number(str, va_arg(args, int), 10, width, precision, flags);
        }
        break;
      case 'u': // Unsigned decimal integer
        if (sub_specifier == 'l') {
          str = _number(str, va_arg(args, unsigned long), 10, width, precision, flags);
        }
        else {
          str = _number(str, va_arg(args, unsigned int), 10, width, precision, flags);
        }
        break;
      case '%':
        *str++ = '%';
        break;
      default:
        if (*fmt) {
          *str++ = *fmt;
        }
        else {
          --fmt; //
        }
        break;
    }
  }

  *str = '\0';
  return str - buf;
}

int printk(unsigned int fg_color, unsigned int bg_color, const char *fmt, ...) {
  int buffer_length = 0;
  int counter = 0;
  int line = 0;
  va_list args;
  va_start(args, fmt);
  buffer_length = vsprintf(string_buffer, fmt, args);
  va_end(args);

  for (; counter < buffer_length; ++counter) {
    unsigned char c = (unsigned char)*(string_buffer + counter);

    switch (c) {
      case '\n':
        _new_line();
        break;
      case '\b':
        _backspace(fg_color, bg_color);
        break;
      case '\t':
        _tab(fg_color, bg_color);
        break;
      default:
        _ascii(c, fg_color, bg_color);
    }
  }

  return buffer_length;
}

/*
 * Helper functions
 */

inline void _adjust_position() {
  if (pos.char_x < 0) {
    pos.char_x = pos.scn_width / pos.char_width - 1;
    pos.char_y -= 1;
  }

  if (pos.char_x >= (pos.scn_width / pos.char_width)) {
    pos.char_y += 1;
    pos.char_x = 0;
  }

  if (pos.char_y < 0) {
    pos.char_y = 0;
  }

  if (pos.char_y >= (pos.scn_height / pos.char_height)) {
    pos.char_y = 0;
  }
}

// `\n` handler
inline void _new_line() {
  pos.char_x = 0;
  pos.char_y += 1;

  _adjust_position();
}

// `\b` handler
inline void _backspace(int fg_color, int bg_color) {
  pos.char_x -= 1;
  _adjust_position();
  putchar(
    pos.fb_addr, pos.scn_width, 
    pos.char_x * pos.char_width, pos.char_y * pos.char_height, 
    fg_color, bg_color, ' ');
}

inline void _tab(int fg_color, int bg_color) {
  int spaces = ((pos.char_x + 4 - 1) & ~(4 - 1)) - pos.char_x;

  while (spaces > 0) {
    putchar(
      pos.fb_addr, pos.scn_width, 
      pos.char_x * pos.char_width, pos.char_y * pos.char_height, 
      fg_color, bg_color, ' ');
    pos.char_x += 1;
    spaces -= 1;
    _adjust_position();
  }
}

inline void _ascii(unsigned char c, int fg_color, int bg_color) {
  putchar(
    pos.fb_addr, pos.scn_width, 
    pos.char_x * pos.char_width, pos.char_y * pos.char_height, 
    fg_color, bg_color, c);
  pos.char_x += 1;
  _adjust_position();
}

int _atoi(const char **s) {
  int i = 0;

  while (is_digit(**s)) {
    i = i * 10 + **s - '0';
    ++(*s);
  }

  return i;
}

/**
 * %[flags][width][.precision][length]specifier
 * http://www.cplusplus.com/reference/cstdio/printf/
 */
int _parse_flags(const char **fmt) {
  int flags = 0;
  
  while (1) {
    if (**fmt == '-') { flags |= LEFT; }
    else if (**fmt == '+') { flags |= PLUS; }
    else if (**fmt == ' ') { flags |= SPACE; }
    else if (**fmt == '#') { flags |= SPECIAL; }
    else {
      break;
    }

    ++(*fmt);
  }

  return flags;
}

int _parse_width(const char **fmt) {
  int width = -1;

  if (is_digit(**fmt)) {
    width = _atoi(fmt);
  }

  // TODO: we omit the `*` case for simplicity.
  return width;
}

int _parse_precision(const char **fmt) {
  int precision = -1;

  if (**fmt == '.') {
    ++(*fmt);

    if (is_digit(**fmt)) { precision = _atoi(fmt); }
    // TODO: we omit the `*` case for simplicity
  }

  return precision;
}

int _parse_sub_specifier(const char **fmt) {
  int specifier = -1;
  // TODO: we omit other specifiers for simplicity
  if (**fmt == 'l') { 
    specifier = **fmt;
    ++(*fmt);
  }

  return specifier;
}

/**
 * type:
 *  - LOWERCASE
 *  - LEFT
 */
char *_number(char *str, long num, int base, int width, int precision, int type) {
  if (base < 2 || base > 36) { return 0; }

  const char *digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (type & LOWERCASE) {
    digits = "0123456789abcdefghijklmnopqrstuvwxyz";
  }

  /* Set sign */
  char sign = 0;
  if ((type & SIGN) && num < 0) {
    sign = '-';
    num = -num;
  }
  else if (type & PLUS) {
    sign = '+';
  }
  else if (type & SPACE) {
    sign = ' ';
  }
  if (sign) { width -= 1; }

  if (type & SPECIAL) {
    if (base == 16) { width -= 2; } // 0x prefix
    else if (base == 8) { width -= 1; } // o prefix
  }

  /* Calculate `num` in `base` and save each digit in `tmp` in reverse order. */
  int i = 0;
  char tmp[64];
  if (num == 0) { tmp[i++] = '0'; }
  else {
    while (num != 0) {
      tmp[i++] = digits[div(num, base)];
    }
  }

  /**
   * For integer specifiers, `precision` specifies the minimun number of digits to be written.
   * The value is not truncated even if the result is longer.
   * */
  if (i > precision) { precision = i; }
  
  /**
   * Minimum number of characters to be printed. If the value to be printed is shorter
   * than this number, the result is padding with blank spaces.
   */
  width -= precision;
  if (!(type & LEFT)) {
    while (width-- > 0) { *str++ = ' '; }
  }

  if (sign) { *str++ = sign; }

  if (type & SPECIAL) {
    if (base == 8) { *str++ = '0'; }
    else if (base == 16) {
      *str++ = '0';
      *str++ = digits[33]; // 0X or 0x
    }
  }

  while (i < precision--) { 
    *str++ = '0';
  }

  while (i-- > 0) {
    *str++ = tmp[i];
  }

  while (width-- > 0) { 
    *str++ = ' ';
  }

  return str;
}