# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/08/05 11:08
# 

cmake_minimum_required(VERSION 3.20)

project(
    mlcrs_config
    LANGUAGES C CXX
)

set(INC
    .
    Private
    Public
    ../Core/Public
    ../Util/Public
)

set(INC_SYS
)

set(SRC
    Private/Config.cpp

    Public/Config.h
)

set(LIB
    OpenZICore
    mlcrs_util
)

mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")