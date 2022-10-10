
cmake_minimum_required(VERSION 3.18.1)

project("avstudydemo2")

file(GLOB player_source player/*.cpp)

# 头文件依赖配置
include_directories(include)

add_library(
        avstudydemo2

        SHARED

        ${player_source}

        log_utils.h
        safe_queue.h
        native-lib.cpp)

# 静态库配置依赖
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -L${CMAKE_SOURCE_DIR}/${CMAKE_ANDROID_ARCH_ABI}")

# 动态库配置依赖
add_library( avcodec SHARED IMPORTED)
set_target_properties( avcodec PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavcodec.so)

add_library( avdevice SHARED IMPORTED)
set_target_properties( avdevice PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavdevice.so)

add_library( avfilter SHARED IMPORTED)
set_target_properties( avfilter PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavfilter.so)

add_library( avformat SHARED IMPORTED)
set_target_properties( avformat PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavformat.so)

add_library( avutil SHARED IMPORTED)
set_target_properties( avutil PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libavutil.so)

add_library( postproc SHARED IMPORTED)
set_target_properties( postproc PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libpostproc.so)

add_library( swresample SHARED IMPORTED)
set_target_properties( swresample PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libswresample.so)

add_library( swscale SHARED IMPORTED)
set_target_properties( swscale PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/${CMAKE_ANDROID_ARCH_ABI}/libswscale.so)

target_link_libraries(
        avstudydemo2
        -landroid
        avutil
        swresample
        avcodec
        avformat
        swscale
        postproc
        avfilter
        OpenSLES
        log)