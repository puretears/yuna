# The building system

[本节源代码](https://github.com/puretears/yuna/blob/master/Makefile)

从这一节开始，我们就要真正走进内核开发的大门了。在开发各种功能组件之前，第一件事，就是为项目编写一套构建脚本。它就像在 IDE 里使用的各种项目模板一样，指引了整个项目的构建和配置过程。只不过，并没有 IDE 为操作系统的内核开发提供模板，我们只好自己写一个了。而用到的工具，就是 `Makefile`。

## 整理项目目录

既然是手工打造的构建模板，编写 Makefile 之前，我们要先整理好项目的目录结构。当前，我们使用的目录结构是这样的：

```shell
+-Root
  +- Documentation
  |
  +- Source
    +- Boot
    |
    +- Kernel
  +- Output
  |
  .bochsrc
  |
  .gitignore
  |
  .build.sh
  |
  cp.sh
  |
  Makefile
```

其中：

* Documentation 是项目的 markdown 文档和图片；
* Source 是源代码目录，Source/Boot 是操作系统引导和加载内核阶段的代码，也就是之前我们编写过的所有内容；Source/Kernel 是内核代码，至于这里该放什么，我们稍后再说；
* Output 是项目生成的各种中间文件、bochs 的日志文件，以及最终的内核文件的存放目录，把它们统一放在一个目录的好处，就是清理起来很方便；
* cp.sh 是把 loader 和 内核拷贝到软盘镜像的脚本，由于我们调整了目录结构，它的内容也做了相应的调整：

```shell
#!/bin/bash

mount Output/boot.img /media -t vfat -o loop
cp -f Output/loader.bin /media
cp -f Output/kernel.bin /media
sync
umount /media
```

其实功能还是一样的，只是调整了 loader.bin 和 kernel.bin 的目录而已。

* 最后剩下的，就是这一节的 Makefile 了；

## 整理 Makefile

### 定义变量

我们从头至尾过一遍其中的内容。首先，用 shell 命令获取了当前目录：

```shell
ROOT:=$(shell pwd)
```

这里，我们用 `$(shell)` 函数，获取了 `pwd` 命令的结果。我们假定执行 `make` 命令的时候，用户所处的目录就是项目的根目录，因此变量 `ROOT` 就是项目根目录的完整路径。在 Makefile 中，要使用 Shell 中某个命令的结果时，就可以这样。当然，make 还提供了很多函数供开发者使用，大家知道这种语法就好，等我们用到的时候，随用随说。

其次，是表达项目工具链的一组变量：

```shell
# Toolchain
ASM=nasm
CXX=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 g++
CC=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 gcc
AS=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 as
LD=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 ld
CP=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 ./cp.sh
OBJCOPY=docker run -v $(ROOT):/home/root -w /home/root --privileged boxue/base:1.1.0 objcopy
```

这里，`CXX / CC / AS / LD` 是 make 约定俗成使用的一些变量名，分别表示 C / C++ / 汇编 / 链接时使用的工具。由于我的开发环境是 Mac，为了使用 GCC 工具链，这里把它们设定成了 Docker 命令。如果大家在 Linux 环境开发，就直接设置成 g++ / gcc / as 和 ld 就好了。至于为什么要使用 GCC 工具链呢？这和稍后，我们在调试内核时使用的方法有关。况且 Linux 内核也是使用 GCC 工具链构建的，一旦我们遇到什么问题，就能直接从 Linux 项目中得到启发和帮助。

除了这四个约定俗成的变量名之外，我们还额外定义了三个用到的：

* `ASM` 这是我们编译引导扇区和 loader 时使用的汇编器，也就是 nasm；
* `CP` 这是安装 loader 和内核的脚本；
* `OBJCOPY` 这是我们删除内核 ELF 格式额外信息的工具，至于为什么要这样做，我们稍后再说，现在大家对这个事情有个印象就好了；

> 通过这些定义我们不难发现，在 Makefile 里，一个好的习惯，就是把用到的各种工具都通过变量表示。因为这些工具在 Makefile 中都不止一次会用到，定义成变量之后，就可以在一个地方集中管理和替换了。

除了定义工具链变量之外，我们还定义了一些表示目标文件的变量：

```shell
# Objects
YUNABOOT=Output/boot.bin Output/loader.bin
YUNAKERNEL=Output/kernel.bin
YUNASYSTEM=Output/system
OBJS=Output/head.o Output/printk.o Output/main.o
```

其中：

* `YUNABOOT` 是引导扇区和 loader；
* `YUNAKERNEL` 是最终要安装到 boot.img 的内核镜像；
* `YUNASYSTEM` 是 ELF 格式的内核镜像；
* `OBJS` 是构成内核镜像的目标文件列表；

至此，关于 Makefile 变量的部分，就说完了。

### 定义 Makefile 规则

然后，是构建规则的部分，我们先来划分一下这个 Makefile 的功能：

* 构建引导扇区、loader 以及内核；
* 清理项目目录；
* 安装 loader 和内核；

为此，我们使用 `.PHONY` 给 Makefile 定义了三个虚拟目标：

```shell
.PHONY: everything clean image
```

其中，

* `everything` 用于完成编译和链接；
* `clean` 用于清理项目目录；
* `image` 用于安装 loader 和内核；

先来看 `everything`，它依赖的，就是我们要构建出来的所有目标文件：

```shell
everything: $(YUNABOOT) $(YUNAKERNEL) $(YUNASYSTEM)
```

那 `$(YUNABOOT) $(YUNAKERNEL) $(YUNASYSTEM)` 中定义的文件又是怎么构建出来的呢？很简单，我们把构建每一个目标的命令写出来就好了：

```shell
Output/boot.bin: Source/Boot/boot.asm Source/Boot/fat12.inc Source/Boot/s16lib.inc
  $(info ========== Building Boot.bin ==========)
  $(ASM) -ISource/Boot -o $@ $<

Output/loader.bin: Source/Boot/loader.asm Source/Boot/fat12.inc Source/Boot/s16lib.inc Source/Boot/display16.inc
  $(info ========== Building Loader.bin ==========)
  $(ASM) -ISource/Boot -o $@ $<

$(YUNAKERNEL): $(YUNASYSTEM)
  $(info ========== Remove extra sections of the kernel ==========)
  $(OBJCOPY) -S -R ".eh_frame" -R ".comment" -O binary $< $@

$(YUNASYSTEM): $(OBJS)
  $(info ========== Linking the kernel ==========)
  $(LD) -z muldefs -o $@ $(OBJS) -T Source/Kernel/kernel.lds

Output/main.o: Source/Kernel/main.c Source/Kernel/lib.h Source/Kernel/printk.h Source/Kernel/font.h
  $(info ========== Compiling main.c ==========)
  $(CC) -mcmodel=large -fno-builtin -ggdb -m64 -c $< -o $@

Output/printk.o: Source/Kernel/printk.c Source/Kernel/lib.h Source/Kernel/printk.h Source/Kernel/font.h
  $(info ========== Compiling printk.c ==========)
  $(CC) -mcmodel=large -fno-builtin -ggdb -m64 -fgnu89-inline -fno-stack-protector -c $< -o $@

Output/head.o: Source/Kernel/head.S
  $(info ========== Building kernel head ==========)
  $(CC) -E $< > Output/_head.s
  $(AS) --64 -o $@ Output/_head.s
```

这次，我们使用了 `make` 中的 `$(info)` 函数，在控制台打印了正在构建的每一个目标。这些目标的内容，有两个我们应该是熟悉的，就是构建 boot.bin 和 loader.bin 的部分：

```shell
Output/boot.bin: Source/Boot/boot.asm Source/Boot/fat12.inc Source/Boot/s16lib.inc
  $(info ========== Building Boot.bin ==========)
  $(ASM) -ISource/Boot -o $@ $<

Output/loader.bin: Source/Boot/loader.asm Source/Boot/fat12.inc Source/Boot/s16lib.inc Source/Boot/display16.inc
  $(info ========== Building Loader.bin ==========)
  $(ASM) -ISource/Boot -o $@ $<
```

至于构建内核的部分，大家可以先无需关心，接下来我们讲到内核结构的时候，会逐一提到它们。

接下来，是清理项目目录的规则。得益于之前定义的变量，这条规则写起来就容易多了。当然，简单起见，我们也可以无脑地清空 Output 目录中的所有文件。但是由于 bochs 调试日志也放在这个目录里，所以具体的做法，就看大家的需要了：

```shell
clean:
  - rm -f $(OBJS) $(YUNABOOT) $(YUNAKERNEL) $(YUNASYSTEM) Output/_head.s
```

最后，是安装 loader 和内核的规则。`image` 没有任何依赖的文件，因此，为了把 loader.bin 和 kernel.bin 写入磁盘镜像，我们需要明确执行 `make image` 才行：

```shell
image:
  dd if=Output/boot.bin of=Output/boot.img bs=512 count=1 conv=notrunc
  $(CP)
```

## What's next

至此，这个 Makefile 就算是初步完成了。大家可以[参考完整的源代码](https://github.com/puretears/yuna/blob/master/Makefile)再来理解下 Makefile 中我们提到的每一部分内容。借助这个文件，我们就有了三种管理项目的功能：

* make everything - 完整编译内核；
* make clean - 清理源代码目录；
* make image - 安装 loader 和内核镜像；

下一节，我们就来实现一个功能简单，结构俱全的内核。
