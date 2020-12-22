# Save and restore cpu context

初始化了 IDT 和 TSS 之后，这一节，我们就该设置 IDT 中每一项的内容了。它的每一个表项，都对应着一个中断或异常处理程序。之前我们说过，在 IA-32e 模式下，每个 IDT 表项是 16 字节。为了方便设置，在 [gate.c](https://github.com/puretears/yuna/blob/master/Source/Kernel/gate.c) 里，我们定义了一个 `set_gate64` 函数：

```c
volatile void set_gate64(
  gate_descriptor *desc, unsigned char attr, unsigned char ist, void *handler) {
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
```

其中：

* `desc` 指向要写入的 IDT 的位置；
* `attr` 是门描述符的属性；
* `ist` 指定保存 CPU 上下文信息时使用的栈指针；
* `handler` 是对应的中断或异常处理程序地址；

`set_gate64` 的功能，就是把参数的信息，拆分成门描述的格式并存入相应的位置，大家不熟悉的话对着门描述符去看下就好，我们就不重复了。有了 `set_gate64` 之后，为了方便创建几种不同的门描述符，我们定义了一组便捷函数。首先是供内核使用的中断门和陷阱门：

```c
inline void set_intr_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0x8E, ist, handler);
}

inline void set_trap_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0x8F, ist, handler);
}
```

其次，是供用户层程序使用的中断门和陷阱门：

```c
inline void set_user_intr_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0xEE, ist, handler);
}

inline void set_user_trap_gate64(unsigned int n, unsigned char ist, void *handler) {
  set_gate64(idt + n, 0xEF, ist, handler);
}
```

这样，我们只要传递中断向量、IST 以及对应的处理程序的地址，就可以方便地设置 IDT 的内容了。

## 第一个异常处理程序：除 0 错误

接下来，我们来实现第 0 号异常向量，也就是除 0 错误的处理。整个过程的源头，是 `vector_init()` 函数。它定义在 [trap.c](https://github.com/puretears/yuna/blob/master/Source/Kernel/gate.c) 里：

```c
void vector_init() {
  set_trap_gate64(0, 1, divide_error);
}
```

那 `divide_error` 的实现又是什么样的呢？

## CPU 的上下文环境

为了理解它，我们要先说下当异常发生时，IA-32e 模式下，CPU 会做的事情。由于异常处理本质上可以理解为函数调用，为了能够返回调用前的位置，CPU 要在栈里保存一些信息，这些信息包括：

```shell
+40 SS
+32 RSP
+24 RFLAGS
+16 CS
+8  RIP
 0  Error Code <- RSP

// OR

+28 SS
+24 RSP
+16 RFLAGS
+8  CS
+0  RIP <- RSP
```

也就是说，当切换到到内核处理程序之后，内核的栈指针指向的位置，已经包含了上面这些内容，其中：

* SS:RSP 是发生异常时程序当时使用的栈指针。在 Intel SDM 第三卷 6.14.2 小节，有关于这个栈指针的说明，切换到异常或中断处理程序之后，CPU 会强制把 RSP 在 8 字节上对齐，也就是说，保存这些信息的栈指针一定是 8 字节对齐的，由于 CPU 已经保存了原始的栈指针，因此这么做不会有任何问题；
* RFLAGS 是发生异常时的 RFLAGS 寄存器；
* CS:RIP 是发生异常时下一条指令的地址；
* Error Code 是和异常相关的错误码。这个错误码并不是我们传统意义上理解的一个表示错误的数字，它是一个带格式的结构。并且，不是所有的异常都默认带有错误码。在 Yuna 的实现里，为了保持内核栈的结构统一，对于没有错误码的异常，我们会在对应的位置设置 0；

但切换到异常处理程序之后，只保存上面这些信息显然是不够的。为了能够回到发生异常的地方继续执行，我们还应该保存 CPU 的数据段寄存器和通用寄存器。这部分功能在 32 位模式下，本来是可以通过 TSS 让 CPU 自动完成的。但到了 IA-32e 模式，我们得自己手工完成。

## What's next

至于究竟是怎么完成的，我们就留到下一节吧。因为距离第一个异常处理函数完成并能在屏幕上显示日志，还有比较长的一段路要走。不如现在稍微休息一会儿，整理下思路。在下一节里，一鼓作气地完成。
