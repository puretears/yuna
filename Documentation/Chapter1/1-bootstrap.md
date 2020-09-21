# Bootstrap

从零开始自己动手写一个操作系统内核的好处，自然不用我多说了吧。既然如此，我们就直奔主题：究竟要怎么开始呢？好在，现如今，无论是文档、还是开发工具，都比当年 Linus 发布 Linux 0.1 的时候完善多了，这也让我们复盘一个操作系统内核的开发过程，变成了可能。

## 准备工作

为了完成接下来的内容，你至少需要准备好以下文档：

* [在这里下载](https://software.intel.com/content/www/us/en/develop/download/intel-64-and-ia-32-architectures-sdm-combined-volumes-1-2a-2b-2c-2d-3a-3b-3c-3d-and-4.html) Intel 提供的关于 Intel 64 和 IA-32 处理器的开发文档。当我们要在内核中实现切换到保护模式、切换到 64 位模式、开启内存分页、中断管理，多任务、APIC 等一系列功能时，这份文档里描述了 Intel 处理器对这些特性的硬件支持细节。

安装好以下工具：

* [bochs](http://bochs.sourceforge.net/)：这是我们在开发内核早期使用的 PC 模拟器，它最大的好处，就是内置了一个 CPU 调试器，可以帮助我们方便地观察 CPU 的执行过程并查看内存的内容。

如果你使用 mac OS，直接执行 `brew install bochs` 安装就好了，不推荐大家直接编译源代码，Bochs 使用的 SDL 库貌似会有一些代码和 mac OS 的 C++ 编译器有不兼容的情况，还要自己修改。

如果你使用 Linux，就最好还是[下载源代码](http://bochs.sourceforge.net/getcurrent.html)之后自行编译安装。并确保在执行 `configure` 命令的时候，至少带上 `--enable-debugger` 参数开启调试器支持（不一定所有 Linux 发行版中的二进制 Bochs 都开启了这个选项），这样才能使用 Bochs 内置的调试器功能。

* [nasm](https://www.nasm.us/)：这是我们使用的汇编工具，相比 Linux 使用的 AT&T 语法，nasm 支持更为简洁的 Intel 汇编语法格式，它也更接近我们在大学时学过的汇编语言。在 mac OS 上直接使用 `brew install nasm` 安装就好了，在 Linux 上，则使用对应发行版的包管理工具安装即可。

* 最后，就是选择一个你自己喜欢的编辑器，我们会用到的代码，主要就是 C 和汇编，以及一点点的 C++，所以任何一个现如今的主流开发者编辑器都可以胜任。

## Hello, Kernel World

接下来，该从哪开始呢？用一个我们程序员都懂的说法，就是从一个内核版的 Hello, World 开始。为此，我们要先回顾以下大学时学到的知识。当计算机加电自检之后，BIOS 会把启动盘的第一个扇区加载到物理地址 0x7C00，并执行其中的代码。于是，我们的内核版 Hello, World，就是在这个引导扇区里，写一段在屏幕上打印文字的程序。

为此，我们可以新建一个 boot.asm 文件。先来看这个程序的结构：

```asm
    org 0x7C00

label_start:
    
times 510 - ($ - $$) db 0
dw 0xAA55
```

其中：

* `org 0x7C00` 是 nasm 的命令，用于把汇编代码的基地址设置成 `0x7C00`，这样，当我们的代码被 BIOS 移动到 `0x7C00` 之后，所有相对寻址的代码就可以正常工作了；
* `label_start` 是代码位置标记，稍后，我们从这里开始添加功能；
* `times 510 - ($ - $$) db 0` 中，`$` 和 `$$` 可以理解成两个指针，`$` 表示当前语句的地址，`$$` 表示代码段的首地址（其实我们也只有唯一的一个代码段）。于是，`510 - ($ - $$)` 就表示除去已经编写的代码，在 510 字节中，还空出来多少字节，我们用 `times` 命令，把这些字节填充 0。之所以要填充 0，是因为我们要确保编译出来的二进制文件大小必须是 512 字节。稍后，我们会直接把它镜像到磁盘的第一个扇区上；
* `dw 0xAA55`，这是引导扇区的结束标记，必须用 `0xAA55` 结尾；

至此，这个内核版 Hello, World 程序的结构就说清楚了。也就是说，我们的代码，最多只能写 510 字节而已。接下来，我们来实现向屏幕打印字符的功能。

首先，我们把 CPU 的常用段寄存器设置成 `0x7C00`，这样做是为了接下来，使用 BIOS 中断 API 的时候，确保内存的读写都是正确的：

```asm
BASE equ 0x7C00

label_start:
    mov ax, cs
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, BASE
```

其次，用 BIOS 中断 API 清屏。在使用 `int 10H` 之前，每一个寄存器设置值的含义，我都写在了注释里。在初始的实模式下，默认的显示器分辨率是 80 x 25。因此，我们把 `CH / CL` 表达的初始位置设置成 0，`DH / DL` 表达的结束位置设置成 25 和 80，就可以把整个屏幕清空了。当然，你也可以不关注这些细节，把下面的代码理解成反正把 `AX / BX / CX / DX` 设置成这些值之后，再用 `int 10H`，就能清屏了也可以，反正我们也不会反复去写这样的代码：

```asm
; Clear screen
; AH = 06H Scroll screen in a specific range
; AL = 0 Clear screen
; BH = 07H 0 - black screen; 7H - white font;
; CH = start column number
; CL = start row number
; DH = end column number
; DL = end row number
mov ax, 0600H
mov bx, 0700H
mov cx, 0
mov dx, 0184FH ; 25 x 80 resolution
int 10H
```

第三，清空屏幕之后，就可以用 BIOS 中断显示字符了：

```asm
; Display on screen: Start booting ...
; AH = 13H
; AL = 01 move the cursor to the end of string
; BH = 0
; BL = 0FH Highlighted white color
; CX = lenth of the string
; DH = start row number
; DL = start column number
; ES:BP = address of the string
mov ax, 1301H
mov bx, 000FH
mov cx, 0014H
mov dx, 0000H
push ax
mov ax, ds
mov es, ax
pop ax
mov bp, booting_message
int 10H
```

这里的重点，就是在调用 `int 10H` 之前，让 `ES:BP` 是字符串的首地址，`CX 是字符串的长度。至于其它的寄存器，则是对字符串显示属性的设定，大家看看注释就好了。

第四，显示完字符串之后，我们就重置软盘驱动器（至于为什么用软盘而不是硬盘，我们稍后再说），并把系统“挂起”在一条 `jmp` 语句上：

```asm
; Reset floppy
; DL = 00H floopy disk
; DL = 80H hard disk
xor ah, ah
xor dl, dl
int 13H

; Hanging the system
jmp $
```

最后，是 `booting_message` 的定义：

```asm
booting_message      db 'Hello, Kernel World!'
times 510 - ($ - $$) db 0
dw 0xAA55
```

看到这，我们就更能理解通过 `$ - $$` 给剩余字节填 0 的含义了吧。

## What's next

至此，我们的第一个内核版的 Hello, World 就完成了，完整的代码[在这里](https://github.com/puretears/yuna/blob/master/Documentation/Chapter1/Execise01/boot.asm)。下一节，我们来看如何通过 Bochs 调试和执行这段代码。
