#!/bin/bash

if [ "$1" = "x86" ];then
    `gcc CMp4Encoder.c libmp4v2_x86.a -o cmp4v2 -lstdc++ -lm`
elif [ "$1" = "ak" ];then
    `arm-anykav200-linux-uclibcgnueabi-gcc CMp4Encoder.c libmp4v2.a -o a -lstdc++ -lm`
else
    echo "no type" 
    exit
fi
