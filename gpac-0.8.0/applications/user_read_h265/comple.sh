#!/bin/bash

arm-hisiv300-linux-uclibcgnueabi-g++ -o mp4_hevc mp4read.cpp  -I../include -I../include/gpac -I../extra_lib/include -L../extra_lib -L/media/jia/hisi/1/gpac-0.8.0/bin/gcc -L/usr/local/lib -lz -lpthread -lgpac_static -lm #-llzm -lGL -lGLU  -ljpeg 
#LIBS -lm -L/usr/local/lib -lGL -lGLU -lX11  -lz -ljpeg -lpng -lpthread -ldl -llzm  -L/usr/lib/x86_64-linux-gnu/mesa 
