#!/bin/bash
ROOT=$(pwd)
IMAGE=$ROOT/Output/boot.img

if [ ! -f "$IMAGE" ]; then
    echo "Creating $IMAGE"
    bximage -mode=create -fd=1.44M -q $IMAGE
fi

echo "========== Building Boot Loader =========="
cd ./Source/Boot
make
cd ../../

echo "========== Building The Kernel =========="
cd ./Source/Kernel
make
cd ../../

echo "========== Dumping Boot Sector =========="
dd if=$ROOT/Source/Boot/Output/boot.bin of=$IMAGE bs=512 count=1 conv=notrunc

echo "========== Installing Loader and Kernel =========="
docker run -v $ROOT:/home/root -w /home/root --privileged boxue/base:1.1.0 ./cp.sh
