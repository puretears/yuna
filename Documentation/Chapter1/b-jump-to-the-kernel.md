# Jump to the kernel

这一节，我们完成切换到内核的最后一部分准备工作。把 loader 切换到保护模式，再切换到 64 位模式（也叫做 IA-32e 模式），最终跳转到内核。

## 切换到保护模式

希望你还记得，之前为了进入 Real Big Mode，我们让 loader.bin 在保护模式逛了一圈。现在还是使用同样的 GDT，我们装载 `gdtr` 寄存器：

```asm
cli
    
db 0x66
lgdt [gdt_ptr]
```

> 注意，接下来的 CPU 设置都必须在关中断的条件下完成。因此，我们使用了 `cli` 指令。

实际上，在保护模式里，不仅内存的管理方式有所变化，中断的执行方式也和实模式不同。简单来说，中断执行的代码，是通过一个叫做 interrupt descriptor 的结构描述的，这些 descriptor 组合起来的结构，叫做 interrupt descriptor table，也就是 IDT。这个表的概念，和我们之前使用的 GDT 是类似的。至于 IDT 的细节，等我们为内核实现中断处理程序的时候再说，大家知道有这么回事儿就好。在 loader.bin 里，我们先设置一个全 0 的替身作为 IDT 就好了，毕竟接下来我们都处于中断关闭的状态，也不会用到 IDT。

为此，在 loader.asm 里，我们添加了下面的代码：

```asm
; ========= Tmp IDE =========
; Because interrupts are disabled during setting up process,
; these dummy IDT entries are enough.
idt:
    times 0x50 dq 0
idt_end:

idt_ptr:
    dw idt_end - idt - 1
    dd idt
```

看到了把，每个 IDT 表项和 GDT 表项一样，也是 8 字节。另外，CPU 也有一个叫做 `idtr` 的寄存器，用于装载 IDT，因此，紧挨着 IDT，我们也仿造 `gdt_ptr` 构建了一个 `idt_ptr`，这样，就可以用类似的方式装载它了：

```asm
db 0x66
lidt [idt_ptr]
```

设置好了 `gdtr` 和 `idtr`，我们重新让 loader.bin 回到保护模式：

```asm
mov eax, cr0
or eax, 1
mov cr0, eax
```

### 用远跳转清空缓存

接下来，为了在保护模式下正常工作，我们要执行一条远跳转指令：

```asm
jmp dword code32_sel:pm_mode_start

[SECTION .s32]
[BITS 32]
pm_mode_start:
```

这个 `jmp` 指令有两个作用：

* 一个是从 16 位代码段跳转到 32 位代码段，让 CPU 工作在 32 位模式；
* 另一个是通过跳转指令，清空 CPU 的指令预取缓存，毕竟实模式预取的指令，在保护模式都没用了；

来到 32 位模式之后，我们要做的第一件事，就是给所有段寄存器都加载上 `data32_desc` 所描述的地址空间：

```asm
pm_mode_start:
    mov ax, data32_sel
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov ss, ax
    mov esp, 7E00H
```

这样，接下来所有汇编指令的内存访问的基地址，就都是 0x00000000 了。

### 判断 CPU 是否支持 64 位模式

设置好了所有段寄存器之后，接下来，我们要判断下当前 CPU 是否支持 64 位模式。这可以借助于 `cpuid` 指令。为此，我们定义了一个 `intel64_available` 函数，它没有参数，如果 CPU 支持 64 位模式返回 1，否则返回 0：

```asm
    ; Return
    ; eax: 1 - available; 0 - unavailable;
intel64_available:
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    setnb al
    jb _long_mode_done
    mov eax, 0x80000001
    cpuid
    bt edx, 29
    setc al
_long_mode_done:
    movzx eax, al
    ret
```

对于 CPU 来说，当使用 `0x80000000` 作为扩展功能号执行 `cpuid` 时，只有返回值大于等于 `0x80000001` 时，CPU 才可能支持 64 位模式。因此，比较之后，我们直接用 `setnb` 命令设置了 `al`。如果，`cpuid` 的返回结果小于 `0x80000001`，就直接设置返回结果并结束就好了。

否则，我们进一步使用 `0x80000001` 扩展功能执行 `cpuid` 指令，这次，`edx` 返回结果中的 bit-29 表示了 CPU 是否支持 64 位模式。置位表示支持，否则不支持。于是，我们直接用 `bt` 测试结果，并把测试结果写入 `al`。这样，就可以和之前不支持 64 位模式时的执行逻辑合并了。

有了这个方法之后，在 loader.asm 中添加下面的代码：

```asm
call intel64_available
test eax, eax
jnz _intel64_available

; We cannot display anything after changing the display mode.
; Just hang up the system
jmp $
```

如果 CPU 不支持 64 位模式，由于我们更改了显示器的显示模式，现在还不能输出任何消息，因此，就直接挂起系统就好了。否则，就跳转到 `_intel64_available` 继续为内核准备数据。

## 构建临时页表

接下来，我们要构建一份在 64 位模式使用的临时页表：

```asm
; -------------- Temporary Page Table --------------
mov dword [0x90000], 0x91007  ; PML4

mov dword [0x91000], 0x92007  ; PDPT

mov dword [0x92000], 0x000083 ; PD
mov dword [0x92008], 0x200083
mov dword [0x92010], 0x400083
mov dword [0x92018], 0x600083
mov dword [0x92020], 0x800083
mov dword [0x92028], 0xa00083
; ------------- End Temporary Page Table ------------
```

页表是什么呢？简单来说，就是一种按固定大小的内存块管理内存的方法，至于这些数据具体是什么意思，由于这个系统稍微有点儿复杂，我们先不在这里展开了，稍后，我们专门用一节的内容来说明这个问题。现在把这些代码添加到 loader.asm 里即可。

## 64 位模式的 GDT

准备好页表之后，我们要把 GDT 替换成 64 位模式下使用的版本。大体上和 32 位保护模式的 GDT 相同，只不过，64 位模式下，CPU 已经不再需要指定 base address，也不再检查 limit，把 segment descriptor 中对应的 bit 设置成 0 即可。为此，在 loader.asm 里，我们创建了一个 `[SECTION gdt64]`：

```asm
[SECTION gdt64]
label_gdt64: dd 0, 0
code64_desc: dd 0x00000000, 0x00209A00
data64_desc: dd 0x00000000, 0x00009200

gdt64_len equ $ - label_gdt64
gdt64_ptr dw gdt64_len - 1
          dd label_gdt64

code64_sel equ code64_desc - label_gdt64
data64_sel equ data64_desc - label_gdt64
```

这里，唯一需要强调的是 `code64_desc`，它的第七字节的 bit-5 设置成了 1，这个 bit 在 Intel SDM-V3 3.4.5 中定义为 `L`，置位的时候，表示 64 位代码段。

定义好了 `gdt64` 之后，我们重新加载 `gdtr`，并更新所有段寄存器：

```asm
lgdt [gdt64_ptr]

mov ax, data64_sel
mov ds, ax
mov es, ax
mov fs, ax
mov gs, ax
mov ss, ax
mov esp, 7E00H
```

## 切换到 64 位模式

完成后，我们设置 `cr4` 寄存器的 bit-5，它决定了我们使用的内存分页模型：

```asm
; Open PAE
mov eax, cr4
bts eax, 5
mov cr4, eax
```

然后，把刚才设置页表的地址，装载到 `cr3` 寄存器：

```asm
Load CR3
mov eax, 90000H
mov cr3, eax
```

再通过设置 MSR 寄存器把 64 IA-32e 模式的寻址方式设置成 4 级页表。这里 `rdmsr / wrmst` 是 Intel CPU 读写 MSR 寄存器的专用指令。当使用 `rdmsr` 读取 MSR 寄存器时，要通过 `ecx` 指定 MSR 寄存器的地址。至于 `0C0000080H` 这个值的含义，稍后我们和分页模式一起说明：

```asm
; Enable long mode
mov ecx, 0C0000080H
rdmsr
bts eax, 8
wrmsr
```

最后，重新设置 `cr0` 寄存器的 bit-0 和 bit-31，开启分页模式：

```asm
; Enable paging
mov eax, cr0
bts eax, 0
bts eax, 31
mov cr0, eax
```

现在，如果一切正常，CPU 就应该工作在 64 位模式了。

## 把控制权转给内核

和之前我们开启保护模式类似，进入 IA-32e 模式之后，我们也要执行一条远跳转指令，清空指令预取缓存：

```asm
jmp code64_sel:KERNEL_OFFSET
```

`code64_sel:KERNEL_OFFSET` 就表示了 IA-32e 模式下，内核加载的首地址，当然，实际上它的起始物理地址仍旧是 1MB 的位置。我们可以在 Bochs 里打个断点 `pb 0x100000`，当 CPU 断下来的时候，结果应该是类似这样的：

```asm
<bochs:1> pb 0x100000
<bochs:2> c
(0) Breakpoint 1, 0x0000000000100000 in ?? ()
Next at t=15919289
(0) [0x000000100000] 0008:0000000000100000 (unk. ctxt): xor eax, eax
```

可以看到，物理地址 0x100000 位置的指令，正是之前编写的内核替身的第一条指令。并且，这条指令的线性地址，已经变成了 `0008:0000000000100000`，这可以确定，CPU 已经工作在 IA-32e 模式了。

## What's next

至此，经历了从引导扇区加载 loader，再从 loader 加载内核，我们用这种“接力加载”的形式，一步步突破了 512 字节的存储限制，1MB 的内存限制，终于来到了内核的大门前。

然而，内核的二进制结构，比现在这个“充数”的 kernel.bin 复杂的多。哪部分是代码，哪部分是已初始化数据，哪部分是未初始化数据，这些部分都分别加载到哪个地址空间，这些内容都需要精确构建在内核二进制文件里。因此，下一节，我们就从构建内核的二进制继续我们的内核探索。
