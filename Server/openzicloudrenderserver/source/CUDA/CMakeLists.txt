# Copyright (c) @ 2022-2025 OpenZI 数化软件, All rights reserved.
#
# Licensed under GNU AFFERO GENERAL PUBLIC LICENSE VERSION 3, (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    https://www.gnu.org/licenses/agpl-3.0.en.html#license-text
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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