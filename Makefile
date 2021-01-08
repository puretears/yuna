
ROOT:=$(shell pwd)
BASE_SYS=boxue/base:1.2.0

# Toolchain
ASM=nasm
CXX=docker run -v $(ROOT):/home/root -w /home/root $(BASE_SYS) g++
CC=docker run -v $(ROOT):/home/root -w /home/root $(BASE_SYS) gcc
AS=docker run -v $(ROOT):/home/root -w /home/root $(BASE_SYS) as
LD=docker run -v $(ROOT):/home/root -w /home/root $(BASE_SYS) ld
CP=docker run -v $(ROOT):/home/root -w /home/root --privileged $(BASE_SYS) ./cp.sh
OC=docker run -v $(ROOT):/home/root -w /home/root $(BASE_SYS) objcopy

# Options
WARNING_FLAGS=-Wall -Wextra -pedantic -Wold-style-cast
COMMON_C_FLAGS=-mcmodel=large -fno-builtin -ggdb -m64 -c -fno-stack-protector -ffreestanding -fno-exceptions
COMMON_CPP_FLAGS=$(COMMON_C_FLAGS) -std=c++2a -fno-rtti 

# Objects
YUNABOOT=Output/boot.bin Output/loader.bin
YUNAKERNEL=Output/kernel.bin
YUNASYSTEM=Output/system
OBJS=Output/head.o Output/entry.o Output/gate.o Output/trap.o Output/lib.o Output/memory.o Output/io.o Output/interrupt.o Output/printk.o Output/main.o 

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
	$(OC) -S -R ".eh_frame" -R ".comment" -O binary $< $@

$(YUNASYSTEM): $(OBJS)
	$(info ========== Linking the kernel ==========)
	$(LD) -z muldefs -o $@ $(OBJS) -T Source/Kernel/kernel.lds

Output/main.o: Source/Kernel/main.cpp Source/Kernel/lib.h Source/Kernel/trap.h \
  Source/Kernel/gate.h Source/Kernel/task.h Source/Kernel/memory.h \
  Source/Kernel/printk.h Source/Kernel/font.h
	$(info ========== Compiling main.c ==========)
	$(CXX) $(COMMON_CPP_FLAGS) $< -o $@

Output/lib.o: Source/Kernel/lib.c Source/Kernel/lib.h
	$(info ========== Compiling lib.c ==========)
	$(CC) $(COMMON_C_FLAGS) $< -o $@

Output/io.o: Source/Kernel/io.cpp Source/Kernel/io.h
	$(info ========== Compiling io.cpp ==========)
	$(CXX) $(COMMON_CPP_FLAGS) $< -o $@

Output/interrupt.o: Source/Kernel/interrupt.cpp Source/Kernel/io.h \
  Source/Kernel/interrupt.h
	$(info ========== Compiling interrupt.cpp ==========)
	$(CXX) $(COMMON_CPP_FLAGS) $< -o $@

Output/memory.o: Source/Kernel/memory.c Source/Kernel/printk.h \
  Source/Kernel/font.h Source/Kernel/memory.h
	$(info ========== Compiling memory.c ==========)
	$(CC) $(COMMON_C_FLAGS) $< -o $@

Output/printk.o: Source/Kernel/printk.c Source/Kernel/lib.h Source/Kernel/printk.h Source/Kernel/font.h
	$(info ========== Compiling printk.c ==========)
	$(CC) $(COMMON_C_FLAGS) $< -o $@

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
	$(CC) $(COMMON_C_FLAGS) -c $< -o $@

Output/trap.o: Source/Kernel/trap.c Source/Kernel/lib.h Source/Kernel/trap.h \
  Source/Kernel/gate.h Source/Kernel/printk.h Source/Kernel/font.h
	$(info ========== Building kernel interrupt and exception modules ==========)
	$(CC) $(COMMON_C_FLAGS) $< -o $@