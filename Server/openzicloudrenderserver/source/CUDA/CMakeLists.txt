# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/07/27 14:16
# 

cmake_minimum_required(VERSION 3.20)
project(mlcrs_cuda)

find_package(CUDA)
if (NOT CUDA_FOUND)
    message(STATUS "CUDA Not Found. Project will not be build.")
endif()

if (CUDA_FOUND)
    set(INC
        .
        Private
        Public
    )
    set(INC_SYS
    )
    set(SRC
        Private/RGBToNV12.cu

        Public/RGBToNV12.cuh
    )
    set(LIB
    )

    mlcrs_include_dirs(${INC})
    # cuda_include_directories(${INC})
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        if (BUILD_WITH_RUNTIME_CRT)
            set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -Xcompiler \"/MDd\"")
        else()
            set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -Xcompiler \"/MTd\"")
        endif()
    else()
        if (BUILD_WITH_RUNTIME_CRT)
            set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -Xcompiler \"/MD\"")
        else()
            set(CUDA_NVCC_FLAGS "${CUDA_NVCC_FLAGS} -Xcompiler \"/MT\"")
        endif()
    endif()
    # set(CUDA_SEPARABLE_COMPILATION ON)
    cuda_add_library(mlcrs_cuda STATIC ${SRC})

    # mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")

    # if (CMAKE_BUILD_TYPE MATCHES "Debug")
    #     target_compile_options(mlcrs_cuda PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler="/MDd">)
    # else()
    #     target_compile_options(mlcrs_cuda PRIVATE $<$<COMPILE_LANGUAGE:CUDA>:-Xcompiler=/MD>)
    # endif()
endif()