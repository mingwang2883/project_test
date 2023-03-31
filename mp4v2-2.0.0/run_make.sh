#!/bin/sh

#./configure --host=mips-linux-uclibc CXXFLAGS=-fPIC CFLAGS=-fPIC CC=mips-linux-uclibc-gnu-gcc CXX=mips-linux-uclibc-gnu-g++ --prefix=$(pwd)/_install
#./configure --host=arm-augentix-linux-uclibcgnueabihf CXXFLAGS=-fPIC CFLAGS=-fPIC CC=arm-augentix-linux-uclibcgnueabihf-gcc CXX=arm-augentix-linux-uclibcgnueabihf-g++ --prefix=$(pwd)/_install
#./configure --host=arm-anykav500-linux-uclibcgnueabi CXXFLAGS=-fPIC CFLAGS=-fPIC CC=arm-anykav500-linux-uclibcgnueabi-gcc CXX=arm-anykav500-linux-uclibcgnueabi-g++ --prefix=$(pwd)/_install
#./configure --host=arm-augentix-linux-uclibcgnueabihf CFLAGS=-fPIC CXXFLAGS="--std=c++98 -fPIC" CC=arm-augentix-linux-uclibcgnueabihf-gcc CXX=arm-augentix-linux-uclibcgnueabihf-g++ --prefix=$(pwd)/_install
./configure --host=arm-anycloud-linux-uclibcgnueabi CFLAGS=-fPIC CXXFLAGS="--std=c++98 -fPIC" CC=arm-anycloud-linux-uclibcgnueabi-gcc CXX=arm-anycloud-linux-uclibcgnueabi-g++ --prefix=$(pwd)/_install

make -j4 && make install

