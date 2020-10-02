#!/bin/bash

mount Output/boot.img /media -t vfat -o loop
cp -f Source/Boot/Output/loader.bin /media
cp -f Source/Kernel/Output/kernel.bin /media
sync
umount /media
