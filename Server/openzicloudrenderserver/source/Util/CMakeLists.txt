# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/07/27 09:45
# 
cmake_minimum_required(VERSION 3.20)

project(
    mlcrs_util
    LANGUAGES C CXX
)

set(INC
    .
    Private
    Public
    ../Core/Public
)

set(INC_SYS

)

set(SRC
    Private/DataConverter.cpp
    Private/SystemTime.cpp

    Public/DataConverter.h
    Public/SystemTime.h
    Public/PicoJson.h
)

set(LIB
    OpenZICore
)
mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")