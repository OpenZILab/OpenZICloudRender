# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/07/26 16:59
# 

cmake_minimum_required(VERSION 3.20)

project(
    mlcrs_libname
    LANGUAGES C CXX
)

set(INC
    .
    Private
    Public
)

set(INC_SYS

)

set(SRC
    Private/ThreadTool.cpp

    Public/ThreadTool.h
)

set(LIB

)

mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")