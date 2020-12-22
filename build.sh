#!/bin/bash
ROOT=$(pwd)
IMAGE=$ROOT/Output/boot.img

if [ ! -f "$IMAGE" ]; then
    echo "Creating $IMAGE"
    bximage -mode=create -fd=1.44M -q $IMAGE
fi

if [ ! -d "$ROOT/Output" ]; then
    midir -p $ROOT/Output
fi 

make clean all
