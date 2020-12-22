# The print family - i

这一节开始，我们为内核实现一套和 C 中 `printf` 家族函数用法类似的 API。第一个要介绍的函数，是 `printk`，它的声明是这样的：

```c
int printk(unsigned int fg_color, unsigned int bg_color, const char *fmt, ...);
```

其中，`fmt` 和 `...` 的部分，和 C 中的 `printf` 用法是一样的，支持各种格式化字符串的方式，并在后面跟上对应的包含值的变量。不同的是，`printk` 还支持通过 `fg_color` 和 `bg_color` 定义打印字符的前景色和背景色。因此，它用起来是类似这样的：

```c
printk(WHITE, BLACK, "[Info] %2.2d: %s", 1, "Hello, world.");
```

在屏幕上，就会显示一行黑底白字：`[Info] 01: Hello, world.` 了。

## 准备工作

为了实现 `printk`，我们要先做一些准备工作。在 printk.h 里，添加下面的代码：

```c
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
```

`string_buffer` 是 print 家族函数内部使用的字符缓冲区，由于内核还没有内存管理单元，因此一个妥协的办法，就是把要显示在屏幕上的内容，用一个全局变量管理起来。`Position` 是管理屏幕信息的一个结构体，其中：

* `scn_width / scn_height` 是屏幕的水平和垂直分辨率，在我们例子中，是 1440 和 900；
* `char_x / char_y` 是当前光标位置，打印函数会在这个位置上显示字符；
* `char_width / char_height` 是每个字符的宽高，在我们的例子中，是 8 和 16；
* `fb_addr` 是 frame buffer 的基地址，也就是 `0xFFFF800000A00000`；
* `fb_length` 是 frame buffer 的长度，在 0x180 模式下，应该是 1440 * 900 * 32 字节；

定义好这些结构之后，我们定义了一个全局变量 `pos`。它被所有 `printk` 调用共享，获取当前的打印位置。然后，在 main.c 里，添加下面的初始化代码：

```c
void Start_Kernel() {
  // ...
  pos.scn_width = 1440;
  pos.scn_height = 900;
  pos.char_x = 0;
  pos.char_y = 0;
  pos.char_width = 8;
  pos.char_height = 16;
  pos.fb_addr = (unsigned int *)0xFFFF800000A00000;
  pos.fb_length = pos.scn_width * pos.scn_height * 4;
}
```

这些属性的含义刚才都解释过，我们就不重复了。在这段代码后面，调用 `printk`，它就会根据这些初始化信息，在屏幕上打印内容了。

## printk 结构

接下来，看下 `printk` 的实现：

```c
int printk(
  unsigned int fg_color, unsigned int bg_color, const char *fmt, ...) {
  int buffer_length = 0;
  int counter = 0;

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
```

我们一行行来说下。首先是局部变量的部分：

```c
int buffer_length = 0;
int counter = 0;

va_list args;
va_start(args, fmt);
buffer_length = vsprintf(string_buffer, fmt, args);
va_end(args);
```

* `buffer_length` 是格式化之后 `printk` 要打印在屏幕上的字符串的长度。其中，格式化字符串的实际操作是在 `vsprintf` 中完成的，它把字符串写入 `string_buffer`，并返回字符串的长度。它和 C 中的同名函数功能也是相同的。这个函数我们稍后再来实现；
* `counter` 是个计数器，记录当前已经显示的字符数量，我们用它控制 `string_buffer` 显示；
* `va_start / va_end` 之间，是解析字符串格式，并构建 `string_buffer` 的过程；

因此，这部分代码执行过后，在 `string_buffer` 里就已经是要显示在屏幕上的完整内容了。接下来，我们用了一个 `for` 循环，逐个显示字符：

```c
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
```

这里，我们定义了几个打印字符的辅助函数：

* `_new_line` 用于打印换行；
* `_backspace` 用于打印退格；
* `_tab` 用于打印制表符；
* `_ascii` 用于打印其它 ASCII 字符；
  
大家现在知道这些辅助函数的功能就好了，稍后我们会逐一看到它们的完整实现。

## What's next

以上，就是这一节的内容。我们了解了整个打印 API 最“外围”部分的实现。下一节，我们就走到 `printk` 的内部，来看看它用到的几个辅助函数的实现。
