#ADD_DEFINITIONS(-std=c++)

INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/rtsp/
    ${CMAKE_CURRENT_SOURCE_DIR}/xutil/
    ${CMAKE_CURRENT_SOURCE_DIR}/common/
    ${CMAKE_CURRENT_SOURCE_DIR}/app/
)

LINK_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

FILE(GLOB SRCS  app/*.cpp common/*.cpp rtsp/*.cpp xutil/*.cpp)
FILE(GLOB HEADS  app/*.h common/*.h rtsp/*.h xutil/*.h )
SET(APP_SRC ${SRCS} ${HEADS})

add_library(rtsp_pusher_server ${APP_SRC})
target_link_libraries(rtsp_pusher_server pthread rt dl libz.a )
