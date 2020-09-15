# Refactor the display

在继续之前，我们要重构下 loader.bin 的显示方法。之所以要这样做，主要有两个原因：

* 一方面，接下来我们会在屏幕上显示大量信息，每次都像下面这样使用 `int 10H` 中断，不仅需要我们自己维护很多寄存器的状态，每次都复制这些代码也着实是麻烦：

```asm
mov ax, 1301H
mov bx, 000FH
mov dx, 0300H
mov cx, 24
push ax
mov ax, ds
mov es, ax
pop ax
mov bp, finish_loading_kernel_prompt
int 10H
```

* 另一方面，当我们为内核切换到保护模式之后，BIOS 中断也就不再可用了，我们需要使用其它方式在屏幕上显示内容，这里先做一些必要的铺垫；

## 从 VESA 和 VBE 说起

说起和计算机现实有关的内容，必须要提到的就是 VESA (Video Electroic Standards Association)。这是一个致力于推广计算机显示标准的非盈利组织。VBE (Video BIOS Extension) 就是这个组织制定的显示规范标准。当然，全面了解这个标准的每一个细节，仍旧是非常复杂的事情，需要我们具备不少专业的图形知识。如果大家感兴趣，可以在 [VBE wiki](https://en.wikipedia.org/wiki/VESA_BIOS_Extensions) 上阅读这个扩展的详细技术规范。当下，我们只要知道，这个标准可以极大简化我们对显示设备的编程接口就好了。

## 最简单的 Frame Buffer

接下来要说的一个概念，是 frame buffer，你可能并不了解它的确切含义，但你或多或少也应该听说过它。究竟什么是 frame buffer 呢？简单来说，它就是屏幕显示内容的内存映像。屏幕上的每一个像素，就对应 frame buffer 中的一个存储单元。简单来说，就是只要我们在 frame buffer 的内存区域中写进去数据，屏幕上就会有对应内容的显示了。

当 Bochs 启动的时候，显示器工作在**字符显示模式**。此时的屏幕分辨率是 80 x 60，也就是 80 行，60 列。这个行列的每个坐标，可以显示一个字符。而我们刚才说的 frame buffer 的首地址，是 0xB8000。

> 严格来说，这个文本模式的 frame buffer 的首地址有两个，分别是黑白屏幕的 0xB0000 和彩色屏幕的 0xB8000。只不过现如今已经很少有黑白屏幕，我们忽略这种情况就好了。

在这个 frame buffer 里，每个存储单元都是两字节，把它们用图形表达出来，就是这样的：

```shell
  15──────◇──────◇──────◇──────┬───────◇──────◇─────◇───────8
  │   BL  │   R  │  G   │   B  │   HL  │  R   │  G  │   B   │
  └───────◇──────◇──────◇──────┴───────◇──────◇─────◇───────┘
  7─────────────────────────────────────────────────────────0
  │                       Character                         │
  └─────────────────────────────────────────────────────────┘
```

其中：

* 低字节是要显示字符的 ASCII 码；
* 高字节是显示属性。它的低 4 bit 表示前景色，高 4-bit 表示背景色。`R / G / B` 分表表示 Red, Green 和 Blue。`HL` 表示高亮显示，`BL` 表示背景闪烁；

也就是说，只要我们从 0xB8000 开始，按照存储单元的规格，每写两个字节，屏幕上就会多出来一个特定显示效果的字符。

## 一个最简单的打印程序库

基于上面这些内容，开发一个字符模式下的打印 API 就很简单了。为此，我们创建了 [display16.inc](https://github.com/puretears/yuna/blob/master/s16lib.inc)。

### disp_ascii

先来看最简单的 `disp_ascii`，它用于在屏幕上显示一个字符：

```asm
; al - The ascii to be displayed
disp_ascii:
    push gs
    push cx
    push edi
    
    mov cx, 0B800H
    mov gs, cx

    mov edi, [global_disp_position]
    mov ah, 0FH
    mov [gs:edi], ax
    add edi, 2

    mov [global_disp_position], edi

    pop edi
    pop cx
    pop gs
    ret
```

这里，简单起见，我们没有使用标准的 C 函数调用传参的约定，而是直接使用了 `al`，把要显示的 ASCII 放在 `al` 里调用 `disp_ascii` 就好了：

```asm
mov al, 'A'
call disp_ascii
```

由于使用了寄存器传参，这里也不存在平衡栈的问题，很方便。了解了它的用法之后，我们来看实现。

首先，是保存和恢复寄存器状态：

```asm
push gs
push cx
push edi

pop edi
pop cx
pop gs
ret
```

在我们的接口函数里，**除了 `ax` 寄存器之外，确保其它寄存器在函数调用前后都不会发生变化**。这是我们编写函数时遵循的一个准则，这里说一次，在其他函数里，就不再重复了。

其次，让 `gs` 寄存器指向 frame buffer 的起始地址：

```asm
mov cx, 0B800H
mov gs, cx
```

第三，构建存储单元，并写入 frame buffer：

```asm
mov edi, [global_disp_position]
mov ah, 0FH
mov [gs:edi], ax
add edi, 2
```

这里，我们把 `ah` 设定成了 `0FH`，表示**黑底高亮白色不闪烁**的显示属性。另外，`global_disp_position` 是一个“全局变量”，用于记录当前 frame buffer 中的偏移位置。这个变量被 display16 中的每一个 API 共享，每显示一个字符，这个偏移值就加 2。

另外，由于之前 boot.asm 中已经打印了一行消息，在 display16.inc 中，我们要从第二行开始显示，因此 `global_disp_position` 的初始值，是 160。

### disp_string

至此，`disp_ascii` 就完成了。理解了这个思路之后，我们来实现打印字符串的函数 `disp_string`：

```asm
; ax - address of string
disp_string:
    push cx
    push gs
    push si

    mov cx, 0B800H
    mov gs, cx
    mov si, ax

_continue_displaying_string:
    lodsb
    cmp al, 0
    jz _disp_string_end
    call disp_ascii
    jmp _continue_displaying_string
_disp_string_end:

    pop si
    pop gs
    pop cx
    ret
```

这次，`ax` 用于传递参数，表示要显示的字符串的首地址。并且，和 C 一样，我们约定字符串以 0 结尾。于是，`disp_string` 的核心逻辑，就是从 `ax` 指定的位置开始，不断使用 `disp_ascii` 打印字符，直到读到 0 为止，就好了。

### disp_al

另外一个


### clear_screen

接下来，是用于清屏的函数 `clear_screen`，它的思路很简单，就是把 80 x 26 中的每一个位置都写上空格就好了。另外，别忘了“清屏”之后，重置 `global_disp_position` 的位置，这样，就可以从屏幕左上角重新写入了：

```asm
; Clean up the screen
clear_screen:
    push cx
    mov dword [global_disp_position], 0
    mov cx, 2000
_clear:
    mov al, ' '
    call disp_ascii
    loop _clear
    mov dword [global_disp_position], 0
    pop cx
    ret
```

### new_line

接下来，是用于换行的函数 `new_line`，它实际上与显示无关，我们只要根据 `global_disp_position` 的值，算出下一行的起始位置就好了，这个计算方法，就是 `global_disp_position / 16` 的商加 1，再乘以 160：

```asm
new_line:
    ; (global_disp_position / 160 + 1) * 160
    push eax
    push ebx
    push edx
    xor edx, edx
    mov eax, [global_disp_position]
    mov ebx, 160
    div ebx
    inc eax
    xor edx, edx
    mul ebx
    mov [global_disp_position], eax
    pop edx
    pop ebx
    pop eax
    ret
```

## Refactor loader.asm

有了上面这些 API 之后，我们就可以重构下 loader.asm 了：

* 一个是把之前所有使用 `int 10H` 显示字符的部分，都换成 `disp_string` 函数；
* 另一个，是之前所有要显示的字符串末尾，都要添加一字节 0；

具体的代码大家可以[看这里](https://github.com/puretears/yuna/blob/master/loader.asm)，我们就不一一列举了。

## What's next

至此，有了这套新的 API，在屏幕上显示内容就变得容易多了，接下来，我们就回到 loader.asm，为内核读取必要的系统信息。
