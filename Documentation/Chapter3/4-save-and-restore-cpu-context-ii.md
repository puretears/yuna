# Save and restore CPU context ii

接着上一节的内容，当通过门描述符来到中断或异常处理程序之后，程序的逻辑大体上是这样的：

* 首先，在 CPU 已经保存的信息基础上，继续保存实际处理中断或异常的函数的地址、CPU 数据段寄存器以及通用寄存器的值；
* 其次，跳转到真正的中断或异常处理程序执行；
* 第三，从栈中恢复保存的 CPU 数据段寄存器和通用寄存器的值；
* 最后，当执行 `iret` 指令的时候，CPU 就会根据之前它自己保存在栈里的数据返回到之前发生成中断或异常的位置继续执行了；

把这个过程写成伪代码，就是这样的：

```c
handler:
  save_real_handler()
  save_segments()
  save_general_registers()

  jump_to_real_handler()
  
  restore_general_registers()
  restore_segment()

  return_from_handler()
```

其中，保存 CPU 寄存器的部分，用汇编实现起来更方便一些，这部分代码，在 [entry.S](https://github.com/puretears/yuna/blob/master/Source/Kernel/entry.S) 里：

```asm
ENTRY(divide_error)
  pushq $0 // Pesudo error code
  pushq %rax
  leaq do_divide_error(%rip), %rax
  xchgq %rax, (%rsp)

error_code:
  pushq %rax
  
  /*
   * Push segment registers are not supported in IA-32e mode.
   * We have to save them into general registers.
   */
  movq %es, %rax
  pushq %rax
  mov %ds, %rax
  pushq %rax

  xorq %rax, %rax

  pushq %rbp
  pushq %rdi
  pushq %rsi
  pushq %rdx
  pushq %rcx
  pushq %rbx
  pushq %r8
  pushq %r9
  pushq %r10
  pushq %r11
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
```

其中，`ENTRY` 是定义在 [linkage.h](https://github.com/puretears/yuna/blob/master/Source/Kernel/linkage.h) 中的一个宏，目的是方便在 entry.S 中定义全局符号：

```c
#define SYMBOL_NAME(name) name
#define SYMBOL_NAME_LABEL(name) name##:

#define ENTRY(name) \
.global SYMBOL_NAME(name); \
SYMBOL_NAME_LABEL(name)
```

这样，当使用 `ENTRY(divide_error)` 之后，就相当于定义并导出了一个函数：

```c
.global divide_error
divide_error:
```

这里，`divide_error` 就是一开始通过 `set_trap_gate64` 安装到 IDT 中的函数。由于除 0 异常没有专门的错误码，一开始我们先压栈数字 0 作为占位符。接下来这段代码的作用，是在栈里保存实际处理异常的函数的地址：

```asm
pushq %rax
leaq do_divide_error(%rip), %rax
xchgq %rax, (%rsp)
```

这里，我们用了个技巧，把 `rax` 寄存器压栈之后，通过 RIP relative 的方式把 `do_divide_error` 函数的地址保存在 `rax` 里，再交换 `rax` 和此时 `rsp` 指向的内存的值，就既复原了 `rax` 又压栈了实际处理异常的函数地址。至于 `do_divide_error`，它是一个这样的 C 函数：

```c
void do_divide_error(unsigned long rsp, unsigned long error_code);
```

实际上，所有的异常处理函数的签名，都是类似这样的，第一个参数是指向 CPU 上下文信息的栈指针，第二个参数是错误码。至于它的实现，我们一会儿再说。接下来的这部分，就是保存 CPU 的数据段寄存器和通用寄存器了：

```asm
error_code:
  pushq %rax
  
  /*
   * Push segment registers are not supported in IA-32e mode.
   * We have to save them into general registers.
   */
  movq %es, %rax
  pushq %rax
  mov %ds, %rax
  pushq %rax

  xorq %rax, %rax

  pushq %rbp
  pushq %rdi
  pushq %rsi
  pushq %rdx
  pushq %rcx
  pushq %rbx
  pushq %r8
  pushq %r9
  pushq %r10
  pushq %r11
  pushq %r12
  pushq %r13
  pushq %r14
  pushq %r15
```

稍后就会看到，`error_code` 是和所有自带错误码的异常处理程序“接轨”的地方。然后，我们保存了 `ds` 和 `es` 这两个数据段寄存器和所有的通用寄存器。至此 CPU 的上下文环境就保存好了。下面的代码，用于调用实际处理异常的函数：

```asm
cld
movq ERRCODE(%rsp), %rsi
movq FUNC(%rsp), %rdx

movq $0x10, %rdi
movq %rdi, %ds
movq %rdi, %es
movq %rsp, %rdi
callq *%rdx
```

这里的 `ERRCODE` 和 `FUNC` 是在 entry.S 里预定好的偏移值：

```asm
FUNC = 0x88
ERRCODE = 0x90
```

这样，就可以快速在栈中取出刚才保存的数据。然后，让 `ds` 和 `es` 装载上内核数据段的 selector。此时，`rdi` 的值是当前的栈顶，`rsi` 的值是错误码，按照 C 在 Linux 上的函数调用约定，这两个寄存器分别对应着函数的前两个参数，而 `rdx` 中也正好是 `do_divide_error` 的地址。因此，用 `callq *%rdx` 指令，就可以调用真正处理除 0 错误的函数了。

> 大家可以[在这里](https://en.wikipedia.org/wiki/X86_calling_conventions)找到 32 位和 64 位模式下，不同编译平台上的函数调用约定。

等我们从 `do_divide_error` 函数返回之后，就应该恢复 CPU 的上下文环境了，这个操作就是保存 CPU 上下文环境的“逆运算”：

```asm
RESTORE_ALL:
  popq %r15
  popq %r14
  popq %r13
  popq %r12
  popq %r11
  popq %r10
  popq %r9
  popq %r8
  popq %rbx
  popq %rcx
  popq %rdx
  popq %rsi
  popq %rdi
  popq %rbp
  popq %rax
  movq %rax, %ds
  popq %rax
  movq %rax, %es
  popq %rax
  addq $0x10, %rsp // Jump FUNC and ERRORCODE
  iretq
```

要注意的是，当恢复了 `rax`，我们把 `rsp` 加了 16，以跳过保存的函数地址和错误码，这时执行 `iret` 指令，CPU 就能利用当前栈中的数据，回到发生异常或中断的地方了。理解了这个逻辑之后，我们为 `RESTORE_ALL` 这个位置定义一个符号：

```asm
ret_from_exception:
ENTRY(ret_from_intr)
  jmp RESTORE_ALL
```

然后，在 `callq *%rdx` 之后，加上这段代码：

```asm
callq *%rdx

jmp ret_from_exception
```

至此，整个内核的除 0 错误处理逻辑，就说完了。最后，我们来看看刚才跳过的 `do_divide_error`：

```c
void do_divide_error(unsigned long rsp, unsigned long error_code) {
  unsigned long *p = (unsigned long *)(rsp + 0x98);
  printk(RED, BLACK,
    "do_divice_error, ERROR_CODE:%#18lx,RSP:%#18lx,RIP:%#18lx",
    error_code, rsp, *p);
  while(1);
}
```

在它的实现里，我们从内核栈中读出了发生异常时的 RIP，并连同错误码以及内核栈指针一起，打印在了屏幕上，然后挂起系统。从某种意义上说栈，这就和 Windows 的蓝屏以及 mac OS 的五国界面是类似的。

## 验证结果

完成这些工作后，确保在 `Start_Kernel` 里加入下面的代码：

```c
void Start_Kernel() {  
  pos.scn_width = 1440;
  pos.scn_height = 900;
  pos.char_x = 0;
  pos.char_y = 0;
  pos.char_width = 8;
  pos.char_height = 16;
  pos.fb_addr = (unsigned int *)0xFFFF800000A00000;
  pos.fb_length = pos.scn_width * pos.scn_height * 4;

  tss_init();
  load_tr(8);

  vector_init();

  int j = 1;
  --j;
  int b = 3 / j;

  while(1);
}
```

我们人为制造了一个除 0 错误，执行 `make && make image` 重新编译安装下，然后在 bochs 里执行，就能看到类似下面这样的结果了：

![save-and-restore-cpu-context-ii-1](Images/save-and-restore-cpu-context-ii-1@2x.jpg)

如何能确定这个结果就是对的呢？首先，记住发生异常的地址：0xffff800000105d6f。然后，在项目根目录执行：

```shell
objdump -D Output/system > Output/log
```

在 `Output/log` 里查看 `Start_Kernel` 的代码，在 0xffff800000105d6f 这个地址处，可以找到这样的指令：

```shell
ffff800000105d6f: f7 7d e8              idivl -24(%rbp)
```

看到了吧，正好是一条除法指令。然后，我们重新执行 bochs，在这条指令上打个断点：

```shell
lb 0xffff800000105d6f
```

> 注意，这次我们要使用 `lb` 打断点，也就是 linear address break 的缩写。

等 CPU 断下来之后，根据 bochs 的提示，我们查看下 `ss:[rbp-24]` 这个地址的值，可以发现是 0，因此可以断定正是这里触发了 CPU 的异常：

```shell
<bochs:1> lb 0xffff800000105d6f
<bochs:2> c
(0) Breakpoint 1, 0xffff800000105d6f in ?? ()
Next at t=149625185
(0) [0x000000105d6f] 0008:ffff800000105d6f (unk. ctxt): idiv eax, dword ptr ss:[rbp-24] ; f77de8
<bochs:3> x /1wd ss:(rbp-24)
[bochs]:
0xffff800000007de0 <bogus+       0>:	0
```

## What's next

以上，就是这一节的内容，应该说，看到这第一条日志，我们着实付出了不少努力。理解了整个过程之后，接下来，我们来实现其它类型的 CPU 异常处理程序。