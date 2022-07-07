cmake_minimum_required(VERSION 3.5.1)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_C_COMPILER arm-anykav200-linux-uclibcgnueabi-gcc)
set(CMAKE_CXX_COMPILER arm-anykav200-linux-uclibcgnueabi-g++)
add_definitions("-Wall -Os -fno-strict-aliasing")
project(SPOOK)

include_directories(${PROJECT_SOURCE_DIR})
include_directories(${PROJECT_SOURCE_DIR}/../../jk_common)
include_directories(${PROJECT_SOURCE_DIR}/../../jk_common/file_operation)
include_directories(${PROJECT_SOURCE_DIR}/../../ak_sdk/include)
link_directories(${PROJECT_SOURCE_DIR}/../../libs/ak_libs/)

FILE(GLOB SPOOK_C spook/*.c)

SET(JIAKE_FILE ${PROJECT_SOURCE_DIR}/../../jk_common/file_operation/jk_file_operation.c)
SET(SPOOK_FILE ${SPOOK_C})

add_executable(spook ${SPOOK_FILE} ${JIAKE_FILE})

target_link_libraries(spook dl pthread m rt plat_thread akuio akispsdk plat_common plat_vi
    plat_ipcsrv akv_encode plat_venc_cb plat_vpss mpi_venc akaudiofilter plat_ai)
