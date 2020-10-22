
ROOT:=$(shell pwd)

# Toolchain
ASM=nasm
CXX=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 g++
CC=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 gcc
AS=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 as
LD=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 ld
CP=docker run -v $(ROOT):/home/root -w /home/root --privileged boxue/base:1.1.0 ./cp.sh
OBJCOPY=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 objcopy

# Objects
YUNABOOT=Output/boot.bin Output/loader.bin
YUNAKERNEL=Output/kernel.bin
YUNASYSTEM=Output/system
OBJS=Output/head.o Output/entry.o Output/gate.o Output/trap.o Output/printk.o Output/main.o

.PHONY: all clean image

all: $(YUNABOOT) $(YUNAKERNEL) $(YUNASYSTEM)

clean:
	- rm -f $(OBJS) $(YUNABOOT) $(YUNAKERNEL) $(YUNASYSTEM) Output/_head.s

image:
	dd if=Output/boot.bin of=Output/boot.img bs=512 count=1 conv=notrunc
	$(CP)

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

Output/main.o: Source/Kernel/main.c Source/Kernel/lib.h Source/Kernel/trap.h \
  Source/Kernel/gate.h Source/Kernel/printk.h Source/Kernel/font.h
	$(info ========== Compiling main.c ==========)
	$(CC) -mcmodel=large -fno-builtin -ggdb -m64 -c $< -o $@

Output/printk.o: Source/Kernel/printk.c Source/Kernel/lib.h Source/Kernel/printk.h Source/Kernel/font.h
	$(info ========== Compiling printk.c ==========)
	$(CC) -mcmodel=large -fno-builtin -ggdb -m64 -fgnu89-inline -fno-stack-protector -c $< -o $@

Output/head.o: Source/Kernel/head.S
	$(info ========== Building kernel head ==========)
	$(CC) -E $< > Output/_head.s
	$(AS) --64 -o $@ Output/_head.s

Output/entry.o: Source/Kernel/entry.S
	$(info ========== Building kernel interrupt entry ==========)
	$(CC) -E $< > Output/_entry.s
	$(AS) --64 -o $@ Output/_entry.s

Output/gate.o: Source/Kernel/gate.c Source/Kernel/gate.h
	$(info ========== Building kernel gate helpers ==========)
	$(CC) -mcmodel=large -fno-builtin -ggdb -m64 -fgnu89-inline -fno-stack-protector -c $< -o $@

Output/trap.o: Source/Kernel/trap.c Source/Kernel/lib.h Source/Kernel/trap.h \
  Source/Kernel/gate.h Source/Kernel/printk.h Source/Kernel/font.h
	$(info ========== Building kernel interrupt and exception modules ==========)
	$(CC) -mcmodel=large -fno-builtin -ggdb -m64 -fgnu89-inline -fno-stack-protector -c $< -o $@