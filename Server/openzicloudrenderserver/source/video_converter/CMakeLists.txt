# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/10/21 11:33
# 


cmake_minimum_required(VERSION 3.20)
project(video_converter VERSION 0.1.0)

add_library(
    video_converter ""
)

target_sources(
    video_converter
        PRIVATE   
            video_converter.cc
        PUBLIC
            video_converter.h
)

target_include_directories(
    video_converter
        PRIVATE   
            ${MLCRS_THIRDPARTY_PATH}/ffmpeg/ffmpeg-n4.4-latest-win64-gpl-shared-4.4/include
)

target_link_libraries(
    video_converter
        PRIVATE   
            ${MLCRS_THIRDPARTY_PATH}/ffmpeg/ffmpeg-n4.4-latest-win64-gpl-shared-4.4/lib/avformat.lib
            ${MLCRS_THIRDPARTY_PATH}/ffmpeg/ffmpeg-n4.4-latest-win64-gpl-shared-4.4/lib/avcodec.lib
            ${MLCRS_THIRDPARTY_PATH}/ffmpeg/ffmpeg-n4.4-latest-win64-gpl-shared-4.4/lib/avutil.lib
)

add_executable(
    video_converter_test video_converter_test.cc
)

target_link_libraries(
    video_converter_test
    PRIVATE
      video_converter
)