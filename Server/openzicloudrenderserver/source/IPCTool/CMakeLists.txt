# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/08/05 10:43
# 
cmake_minimum_required(VERSION 3.20)

project(
    mlcrs_ipctool
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
    Private/IPCTypes.cpp

    Public/IPCTypes.h
)

set(LIB

)

mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")