# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/08/31 21:21
# 

cmake_minimum_required(VERSION 3.20)
project(mlcrs_avencoder VERSION 0.1.0)



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
    ../CUDA/Public
    ../Config/Public
    ../Util/Public
    ../Core/Public
    ../RHI/Public
    ../D3D11RHI/Public
    ../D3D12RHI/Public
    ../VulkanRHI/Public
    ../video_converter
    ${MLCRS_THIRDPARTY_PATH}/NvEncoder/Public
)

find_package(CUDA)
set(INC_SYS
    ${CUDA_INCLUDE_DIRS}
)

set(SRC
    Private/VideoEncoder.cpp
    Private/VideoEncoderNvEnc.cpp
)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(SRC
        ${SRC}
        Private/CudaConverter.cpp
        Public/CudaConverter.h
        Private/D3D12/D3D12CudaConverter.cpp
        Public/D3D12/D3D12CudaConverter.h
    )
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DWEBRTC_LINUX=1)
    add_definitions(-DWEBRTC_POSIX=1)
    add_definitions(-DWEBRTC_VERSION=84)
    set(SRC
        ${SRC}
        Private/Vulkan/VulkanCudaConverter.cpp
        Public/Vulkan/VulkanCudaConverter.h
    )
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    set(LIB
        OpenZICore
        mlcrs_cuda
        mlcrs_config
        mlcrs_util
        mlcrs_nvencoder
        mlcrs_d3d11rhi
        mlcrs_d3d12rhi
        # VideoEncoderNvEnc use delayimp.lib
        delayimp
    )
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    set(LIB
        OpenZICore
        mlcrs_cuda
        mlcrs_config
        mlcrs_util
        mlcrs_nvencoder
        mlcrs_d3d11rhi
        mlcrs_d3d12rhi
        mlcrs_vulkanrhi
)
endif()


option(WITH_WATER_MARK "Whether to compile program with water mark" ON)

if (WITH_WATER_MARK)
    add_definitions(-DMLCRS_WITH_WATER_MARK=1)
    set(INC
        ${INC}
        ${MLCRS_THIRDPARTY_PATH}/OpenCV/include
    )
    set(SRC
        ${SRC}
        Private/WaterMark/LogoPixels.cpp
        Private/WaterMark/LogoPixels.h
    )

    if (CMAKE_SYSTEM_NAME MATCHES "Windows")
        set(LIB
            ${LIB}
            debug ${MLCRS_THIRDPARTY_PATH}/OpenCV/lib/Debug/opencv_world455d.lib
            optimized ${MLCRS_THIRDPARTY_PATH}/OpenCV/lib/Release/opencv_world455.lib
        )
    elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(LIB
            ${LIB}
            debug ${MLCRS_THIRDPARTY_PATH}/OpenCV/lib/Debug/libopencv_world.a
            optimized ${MLCRS_THIRDPARTY_PATH}/OpenCV/lib/Release/libopencv_world.a
        )
    endif()
else()
    add_definitions(-DMLCRS_WITH_WATER_MARK=0)
endif()


mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE video_converter
)