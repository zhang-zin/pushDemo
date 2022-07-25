cmake_minimum_required(VERSION 3.18.1)

project("pushdemo")

include_directories(include)

add_subdirectory(${CMAKE_SOURCE_DIR}/librtmp)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/libs/x264/${ANDROID_ABI}")

add_library(
        pushdemo
        SHARED
        VideoChannel.cpp
        native-lib.cpp)

find_library(
        log-lib
        log)

target_link_libraries(
        pushdemo
        rtmp
        x264
        ${log-lib})