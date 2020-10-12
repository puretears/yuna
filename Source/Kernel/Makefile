kernel_src:=$(shell pwd)

CXX=docker run -w /home/root -v $(kernel_src):/home/root -w /home/root --privileged boxue/base:1.1.0 g++
CC=docker run -w /home/root -v $(kernel_src):/home/root -w /home/root --privileged boxue/base:1.1.0 gcc
AS=docker run -w /home/root -v $(kernel_src):/home/root -w /home/root --privileged boxue/base:1.1.0 as
LD=docker run -w /home/root -v $(kernel_src):/home/root -w /home/root --privileged boxue/base:1.1.0 ld
OBJCOPY=docker run -w /home/root -v $(kernel_src):/home/root -w /home/root --privileged boxue/base:1.1.0 objcopy

output_dir=Output
objects=$(output_dir)/head.o $(output_dir)/printk.o $(output_dir)/main.o

$(output_dir)/kernel.bin: $(output_dir)/system
	$(OBJCOPY) -S -R ".eh_frame" -R ".comment" -O binary $< $@

$(output_dir)/system: $(objects)
	$(LD) -b elf64-x86-64 -z muldefs -o $@ $(objects) -T kernel.lds

$(output_dir)/main.o: main.c lib.h printk.h font.h
	$(CC) -mcmodel=large -fno-builtin -ggdb -m64 -c $< -o $@

$(output_dir)/printk.o: printk.c lib.h printk.h font.h
	$(CC) -mcmodel=large -fno-builtin -ggdb -m64 -fgnu89-inline -fno-stack-protector -c $< -o $@

$(output_dir)/head.o: head.S
	$(CC) -E $< > $(output_dir)/_head.s
	$(AS) --64 -o $@ $(output_dir)/_head.s

.PHONY: clean
clean:
	- rm $(output_dir)/*
