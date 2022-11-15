#./configure --prefix=$PWD/_install --cc=arm-himix100-linux-gcc --cxx=arm-himix100-linux-g++ --extra-cflags=-I/home/wangming/gpac-0.8.0/extra_lib/include/zlib \ 
#--extral-ldflags=-L/home/wangming/gpac-0.8.0/extra_lib/lib/hisi_gcc --disable-x11-shm --use-zlib=local

# cp chipsdk_lib gcc #
./configure --prefix=$PWD/_install --cc=arm-augentix-linux-uclibcgnueabihf-gcc --cxx=arm-augentix-linux-uclibcgnueabihf-g++ \
    --extra-cflags=-fPIC --disable-x11-shm --use-zlib=local

#./configure --prefix=$PWD/_install --cc=gcc --cxx=g++ --extra-cflags=-I$PWD/extra_lib/include/zlib \
#--extral-ldflags=-L$PWD/extra_lib/lib/agtx_gcc --disable-x11-shm --use-zlib=local
