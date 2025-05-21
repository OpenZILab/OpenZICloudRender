# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/10/20 15:34
# 

cmake_minimum_required(VERSION 3.20)
project(mlcrs_rhi VERSION 0.1.0)



# Select C++17 as the standard for C++ projects.
set(CMAKE_CXX_STANDARD 17)
# If C++17 is not available, downgrading to an earlier standard is NOT OK.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Do not enable compiler specific language extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

set(INC
    .
    Private
    Public
    ../Config/Public
    ../Core/Public
    ${MLCRS_THIRDPARTY_PATH}/NvEncoder/Public
)

set(SRC
    Private/DynamicRHI.cpp

    Public/DynamicRHI.h
    Public/RHIResources.h
)

set(LIB
    OpenZICore
    mlcrs_config
)

option(ENABLE_D3D11 "Whether to compile program to support d3d11" ON)

if (ENABLE_D3D11)
    add_definitions(-DMLCRS_ENABLE_D3D11=1)
else()
    add_definitions(-DMLCRS_ENABLE_D3D11=0)
endif()

option(ENABLE_D3D12 "Whether to compile program to support d3d12" ON)

if (ENABLE_D3D12)
    add_definitions(-DMLCRS_ENABLE_D3D12=1)
else()
    add_definitions(-DMLCRS_ENABLE_D3D12=0)
endif()

option(ENABLE_VULKAN "Whether to compile program to support vulkan" OFF)

if (ENABLE_VULKAN)
    add_definitions(-DMLCRS_ENABLE_VULKAN=1)
else()
    add_definitions(-DMLCRS_ENABLE_VULKAN=0)
endif()

mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")