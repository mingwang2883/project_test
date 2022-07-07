#!/bin/sh
./configure --host=mips-linux-gnu CXXFLAGS=-fPIC CFLAGS=-fPIC CC=gcc CXX=g++ --prefix=$(pwd)/_install

