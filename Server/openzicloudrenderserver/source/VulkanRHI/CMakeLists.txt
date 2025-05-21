# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/10/21 11:33
# 


cmake_minimum_required(VERSION 3.20)
project(mlcrs_vulkanrhi VERSION 0.1.0)

# add_definitions(-DVK_USE_PLATFORM_XCB_KHR)
# add_definitions(-DVK_USE_PLATFORM_XLIB_KHR)

# Select C++17 as the standard for C++ projects.
set(CMAKE_CXX_STANDARD 17)
# If C++17 is not available, downgrading to an earlier standard is NOT OK.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Do not enable compiler specific language extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(STATUS "Vulkan Not Found.")
else()
endif()

set(INC
    .
    Private
    Public
    ../Config/Public
    ../Core/Public
    ../RHI/Public
    ${MLCRS_THIRDPARTY_PATH}/NvEncoder/Public
)

set(SRC
    Private/VulkanDynamicRHI.cpp
    Private/VulkanResources.cpp

    Public/VulkanDynamicRHI.h
    Public/VulkanResources.h
)


set(LIB
    OpenZICore
    mlcrs_config
    mlcrs_rhi
    # ${Vulkan_LIBRARIES}
)

mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")