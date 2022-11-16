#!/bin/sh
./configure --host=arm-anykav500-linux-uclibcgnueabi CXXFLAGS=-fPIC CFLAGS=-fPIC CC=arm-anykav500-linux-uclibcgnueabi-gcc CXX=arm-anykav500-linux-uclibcgnueabi-g++ --prefix=$(pwd)/_install

