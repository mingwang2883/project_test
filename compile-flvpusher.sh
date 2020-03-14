#! /usr/bin/env bash
# compile-flvpusher.sh

haisi_v200_cc="arm-hisiv300-linux-uclibcgnueabi-gcc"
haisi_v200_cxx="arm-hisiv300-linux-uclibcgnueabi-g++"

ABS_DIR="$(cd "$(dirname "$0")"; pwd)"

if [ "$1" = "hisi" ];then
    [ ! -d "$ABS_DIR/build_arm" ] && mkdir "$ABS_DIR/build_arm"
    cd "$ABS_DIR/build_arm"
else
    [ ! -d "$ABS_DIR/build" ] && mkdir "$ABS_DIR/build"
    cd "$ABS_DIR/build"
fi

if [ "$1" = "hisi" ];then
    #CC=$haisi_v200_cc CXX=$haisi_v200_cxx cmake .. && make && exit 0
    CXX=$haisi_v200_cxx cmake .. && make && exit 0
else
    cmake .. && make && exit 0
fi

exit 1
