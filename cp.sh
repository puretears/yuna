#!/bin/bash

mount Output/boot.img /media -t vfat -o loop
cp Output/loader.bin /media
sync
umount /media
