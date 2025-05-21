# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/07/29 12:08
# 
cmake_minimum_required(VERSION 3.20)
project(mlcrs_nvencoder)

set(INC
    .
    Private
    Public
)

find_package(CUDA)

set(INC_SYS
    ${CUDA_INCLUDE_DIRS}
)

set(SRC
    Private/NvEncoder.cpp
    Private/NvEncoderCuda.cpp
    Private/NvEncoderOutputInVidMemCuda.cpp

    Public/NvCodecUtils.h
    Public/NvEncodeAPI.h
    Public/NvEncoder.h
    Public/NvEncoderCLIOptions.h
    Public/NvEncoderCuda.h
    Public/NvEncoderOutputInVidMemCuda.h
)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(SRC
        ${SRC}
        Private/NvEncoderD3D11.cpp
        Private/NvEncoderD3D12.cpp
        Private/NvEncoderOutputInVidMemD3D11.cpp
        Public/NvEncoderD3D11.h
        Public/NvEncoderD3D12.h
        Public/NvEncoderOutputInVidMemD3D11.h
    )
endif()

if(WIN32)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(NVENCODEAPI_LIB ${CMAKE_CURRENT_SOURCE_DIR}/Lib/x64/nvencodeapi.lib)
    else()
        set(NVENCODEAPI_LIB ${CMAKE_CURRENT_SOURCE_DIR}/Lib/Win32/nvencodeapi.lib)
    endif()
else ()
    set(NVENCODEAPI_LIB ${CMAKE_CURRENT_SOURCE_DIR}/Lib/Linux/libnvcuvid.so)
    set(NVENCODEAPI_LIB ${CMAKE_CURRENT_SOURCE_DIR}/Lib/Linux/libnvidia-encode.so)
    # find_library(NVENCODEAPI_LIB nvidia-encode)
endif()

set(LIB
    ${CUDA_CUDA_LIBRARY}
    ${NVENCODEAPI_LIB}
)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(LIB
        ${LIB}
        d3d11.lib
        d3d12.lib
        dxgi.lib
        d3dcompiler.lib
    )
endif()

mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")