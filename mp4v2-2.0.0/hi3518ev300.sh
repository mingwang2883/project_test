#!/bin/sh
./configure --host=arm-himix100-linux CXXFLAGS=-fPIC CFLAGS=-fPIC CC=arm-himix100-linux-gcc CXX=arm-himix100-linux-g++ --prefix=$(pwd)/_install

