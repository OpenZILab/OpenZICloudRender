# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/07/27 09:22
# 

add_subdirectory(AVEncoder)
add_subdirectory(Config)
add_subdirectory(Core)
add_subdirectory(CUDA)
add_subdirectory(IPCTool)
add_subdirectory(PixelStreaming)
add_subdirectory(RHI)
add_subdirectory(D3D11RHI)
add_subdirectory(D3D12RHI)
add_subdirectory(video_converter)
if (CMAKE_SYSTEM_NAME MATCHES "Linux")
add_subdirectory(VulkanRHI)
endif()
add_subdirectory(Util)

option(BUILD_WITH_CRASHPAD "use crashpad if true" ON)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")

    set(INC
        .
        Core/Public
        Config/Public
        CUDA/Public
        IPCTool/Public
        Util/Public
        AVEncoder/Public
        PixelStreaming/Public
        RHI/Public
        D3D11RHI/Public
        D3D12RHI/Public
        ${MLCRS_THIRDPARTY_PATH}/NvEncoder/Public
        ${MLCRS_THIRDPARTY_PATH}/OpenCV/include
        ${MLCRS_THIRDPARTY_PATH}/OpenSSL/include
        ${MLCRS_THIRDPARTY_PATH}/libcurl/include
        # ${MLCRS_THIRDPARTY_PATH}/libOpus/include
        # ${MLCRS_THIRDPARTY_PATH}/WebRTC/include
        # ${MLCRS_THIRDPARTY_PATH}/WebRTC/include/third_party/abseil-cpp
    )
    find_package(CUDA REQUIRED)
    if (NOT CUDA_FOUND)
        message(STATUS "CUDA Not Found. Project will not be build.")
    else()
        # install(FILES ${CUDA_LIBRARIES} DESTINATION ${CMAKE_INSTALL_LIBDIR})
    endif()
    set(INC_SYS
        ${CUDA_INCLUDE_DIRS}
        ${MLCRS_THIRDPARTY_PATH}/zlib/include
    )


    set(SRC
        main.cpp
        # WindowsDeskTopApp.cpp
        # ../resource/OpenZICloudRenderServer.rc
    )

    set(LIB
        
        mlcrs_config
        mlcrs_cuda
        mlcrs_avencoder
        mlcrs_ipctool
        mlcrs_util

        mlcrs_nvencoder
        mlcrs_pixelstreaming
        mlcrs_d3d11rhi
        mlcrs_d3d12rhi
    )

    if (MSVC)
        set(COMPANY_NAME    "Cengzi Technology") # 成都曾自科技有限公司
        set(PRODUCT_NAME    "OpenZI Cloud Render Server")
        set(LegalCopyRight  "Copyright by Cengzi Technology Co., Ltd. All Rights Reserved.")
        set(VERSION_MAJOR   1)
        set(VERSION_MINOR   1)
        set(VERSION_PATCH   0)
        set(VERSION_REVISION 0)
        set(RESOURCE_FILE "${CMAKE_SOURCE_DIR}/resource/OpenZICloudRenderServer.rc")
        configure_file("${CMAKE_SOURCE_DIR}/resource/ResourceFileTemplate.rc.in"
                    "${RESOURCE_FILE}")
    endif()

    add_executable(OpenZICloudRenderServer ${SRC} ../resource/OpenZICloudRenderServer.rc)

    if (BUILD_WITH_CRASHPAD)
        set(INC
            ${INC}
            ${MLCRS_THIRDPARTY_PATH}/Crashpad/include
            ${MLCRS_THIRDPARTY_PATH}/Crashpad/include/third_party/mini_chromium
        )
        set(LIB
            ${LIB}
            debug ${MLCRS_THIRDPARTY_PATH}/Crashpad/lib/Windows/Debug/common.lib
            debug ${MLCRS_THIRDPARTY_PATH}/Crashpad/lib/Windows/Debug/client.lib
            debug ${MLCRS_THIRDPARTY_PATH}/Crashpad/lib/Windows/Debug/util.lib
            debug ${MLCRS_THIRDPARTY_PATH}/Crashpad/lib/Windows/Debug/base.lib
            optimized ${MLCRS_THIRDPARTY_PATH}/Crashpad/lib/Windows/Release/common.lib
            optimized ${MLCRS_THIRDPARTY_PATH}/Crashpad/lib/Windows/Release/client.lib
            optimized ${MLCRS_THIRDPARTY_PATH}/Crashpad/lib/Windows/Release/util.lib
            optimized ${MLCRS_THIRDPARTY_PATH}/Crashpad/lib/Windows/Release/base.lib
        )
        target_compile_definitions(
            OpenZICloudRenderServer
            PRIVATE BUILD_WITH_CRASHPAD
        )
    endif()
    mlcrs_include_dirs(${INC})
    mlcrs_include_dirs_sys(${INC_SYS})
    add_cc_flags_custom_test(OpenZICloudRenderServer)


    if (CMAKE_BUILD_TYPE MATCHES "Debug")
        if (BUILD_WITH_RUNTIME_CRT)
            target_compile_options(
                OpenZICloudRenderServer
                PRIVATE "/MDd"
            )
        else()
            target_compile_options(
                OpenZICloudRenderServer
                PRIVATE "/MTd"
            )
        endif()
    else()
        target_compile_options(
            OpenZICloudRenderServer
            PRIVATE "/Zi"
        )
        if (BUILD_WITH_RUNTIME_CRT)
            target_compile_options(
                OpenZICloudRenderServer
                PRIVATE "/MD"
            )
        else()
            target_compile_options(
                OpenZICloudRenderServer
                PRIVATE "/MT"
            )
        endif()
        target_link_options(
            OpenZICloudRenderServer
            PRIVATE "/DEBUG:FULL"
        )
    endif()
    add_custom_command(
        TARGET OpenZICloudRenderServer
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MLCRS_THIRDPARTY_PATH}/Crashpad/bin/Windows/crashpad_handler.exe ${MLCRS_BINARY_DIR}/source/${CMAKE_BUILD_TYPE}/crashpad_handler.exe
        COMMENT "拷贝crashpad_handler到可执行程序目录，从:${MLCRS_THIRDPARTY_PATH}/Crashpad/bin/Windows到${MLCRS_BINARY_DIR}/source/${CMAKE_BUILD_TYPE}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${MLCRS_BINARY_DIR}/source/Release/OpenZICloudRenderServer.exe D:/CloudSpace/FiveTwo/Programs/CloudRenderServer/OpenZICloudRenderServer.exe
    )

    target_link_libraries(OpenZICloudRenderServer ${LIB})
    set_target_properties(OpenZICloudRenderServer PROPERTIES LINK_FLAGS "/ignore:4099")
    option(BUILD_WITH_ADMINISTRATOR "是否编译exe为默认管理员权限启动" OFF)

    if (BUILD_WITH_ADMINISTRATOR)
        set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /level='requireAdministrator'" )
    endif()
    # set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS" )
    set ( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE" )
elseif (CMAKE_SYSTEM_NAME MATCHES "Linux")

    add_definitions(-DWEBRTC_LINUX=1)
    add_definitions(-DWEBRTC_POSIX=1)
    add_definitions(-DWEBRTC_VERSION=84)
    add_definitions(-DCURL_STATICLIB -DBUILDING_LIBCURL -DUSE_IPV6)
    add_definitions(-DOpenZI_DUMP_TEXTURE_PROCESSTIME=0)

    find_package(Vulkan REQUIRED)

    set(INC
        .
        Core/Public
        Config/Public
        CUDA/Public
        IPCTool/Public
        Util/Public
        AVEncoder/Public
        PixelStreaming/Public
        PixelStreaming/Private
        RHI/Public
        VulkanRHI/Public
        D3D11RHI/Public
        D3D12RHI/Public
        ${MLCRS_THIRDPARTY_PATH}/NvEncoder/Public
        ${MLCRS_THIRDPARTY_PATH}/OpenCV/include
        ${MLCRS_THIRDPARTY_PATH}/OpenSSL/Linux/include
        ${MLCRS_THIRDPARTY_PATH}/libcurl/Linux/include
        ${MLCRS_THIRDPARTY_PATH}/WebRTC/include
        ${MLCRS_THIRDPARTY_PATH}/WebRTC/include/third_party/abseil-cpp
        ${MLCRS_THIRDPARTY_PATH}/zlib/Linux/include

    )
    find_package(CUDA)
    if (NOT CUDA_FOUND)
        message(STATUS "CUDA Not Found. Project will not be build.")
    endif()
    set(INC_SYS
        ${CUDA_INCLUDE_DIRS}
    )
    set(SRC
        LinuxMain.cpp
    )


    set(LIB
        OpenZICore
        mlcrs_util
        mlcrs_pixelstreaming
        Vulkan::Vulkan
    )

    mlcrs_include_dirs(${INC})
    mlcrs_include_dirs_sys(${INC_SYS})
    add_cc_flags_custom_test(OpenZICloudRenderServer)
    add_executable(OpenZICloudRenderServer ${SRC})

    target_link_libraries(OpenZICloudRenderServer ${LIB})
endif()

add_executable(split_headers split_headers.cc)