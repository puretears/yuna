
ROOT:=$(shell pwd)

# Toolchain
ASM=nasm
CXX=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 g++
CC=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 gcc
AS=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 as
LD=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 ld
CP=docker run -v $(ROOT):/home/root -w /home/root boxue/base:1.1.0 ./cp.sh
OBJCOPY=docker run -v $(ROOT):/home/root -w /home/root --privileged boxue/base:1.1.0 objcopy

YUNABOOT=Output/boot.bin Output/loader.bin
YUNAKERNEL=Output/kernel.bin
YUNASYSTEM=Output/system

OBJS=Output/head.o Output/printk.o Output/main.o

.PHONY: everything clean image

everything: $(YUNABOOT) $(YUNAKERNEL) $(YUNASYSTEM)

clean:
	- rm -f $(OBJS) $(YUNAKERNEL) $(YUNAKERNEL) $(YUNASYSTEM) Output/_head.s

image:
	dd if=Output/boot.bin of=Output/boot.img bs=512 count=1 conv=notrunc
	$(CP)

Output/boot.bin: Source/Boot/boot.asm Source/Boot/fat12.inc Source/Boot/s16lib.inc
	$(ASM) -ISource/Boot -o $@ $<

Output/loader.bin: Source/Boot/loader.asm Source/Boot/fat12.inc Source/Boot/s16lib.inc Source/Boot/display16.inc
	$(ASM) -ISource/Boot -o $@ $<

$(YUNAKERNEL): $(YUNASYSTEM)
	$(OBJCOPY) -S -R ".eh_frame" -R ".comment" -O binary $< $@

$(YUNASYSTEM): $(OBJS)
	$(LD) -b elf64-x86-64 -z muldefs -o $@ $(OBJS) -T Source/Kernel/kernel.lds

Output/main.o: Source/Kernel/main.c Source/Kernel/lib.h Source/Kernel/printk.h Source/Kernel/font.h
	$(CC) -mcmodel=large -fno-builtin -ggdb -m64 -c $< -o $@

Output/printk.o: Source/Kernel/printk.c Source/Kernel/lib.h Source/Kernel/printk.h Source/Kernel/font.h
	$(CC) -mcmodel=large -fno-builtin -ggdb -m64 -fgnu89-inline -fno-stack-protector -c $< -o $@

Output/head.o: Source/Kernel/head.S
	$(CC) -E $< > Output/_head.s
	$(AS) --64 -o $@ Output/_head.s