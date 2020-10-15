#!/bin/bash

mount Output/boot.img /media -t vfat -o loop
cp -f Output/loader.bin /media
cp -f Output/kernel.bin /media
sync
umount /media
