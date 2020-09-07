#!/bin/bash
IMAGE=Output/boot.img

if [ ! -f "$IMAGE" ]; then
    echo "Creating $IMAGE"
    bximage -mode=create -fd=1.44M -q Output/boot.img
fi

echo "Compiling boot.asm ..."
nasm boot.asm -o Output/boot.bin

echo "Compiling loader.asm ..."
nasm loader.asm -o Output/loader.bin

echo "Compiling kernel.asm ..."
nasm kernel.asm -o Output/kernel.bin

echo "Dumping boot sector ..."
dd if=Output/boot.bin of=$IMAGE bs=512 count=1 conv=notrunc

echo "Installing loader ..."
docker run -it -w /home/root -v $HOME/Projects/Yuna:/home/root --privileged boxue/base:1.1.0 ./cp.sh
