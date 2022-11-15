#!/bin/sh

./configure --host=mips-linux-gnu CXXFLAGS=-fPIC CFLAGS=-fPIC CC=mips-linux-uclibc-gnu-gcc CXX=mips-linux-uclibc-gnu-g++ --prefix=$(pwd)/_install

