# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/08/25 18:05
# 

cmake_minimum_required(VERSION 3.20)

project(
    mlcrs_pixelstreaming
    LANGUAGES C CXX
)

if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DWEBRTC_WIN=1)
    add_definitions(-DWEBRTC_VERSION=84)
    add_definitions(-DABSL_ALLOCATOR_NOTHROW=1)
    add_definitions(-DCURL_STATICLIB -DBUILDING_LIBCURL -DUSE_IPV6)
    add_definitions(-DOpenZI_DUMP_TEXTURE_PROCESSTIME=0)
    set(INC
        .
        Private
        Public
        ../Core/Public
        ../Core/Public/Containers
        ../Core/Public/MessageCenter
        ../AVEncoder/Public
        ../Config/Public
        ../Util/Public
        ../IPCTool/Public
        ../RHI/Public
        ../D3D11RHI/Public
        ../D3D12RHI/Public
        ${MLCRS_THIRDPARTY_PATH}/OpenSSL/include/openssl
        ${MLCRS_THIRDPARTY_PATH}/libOpus/include
    )

    set(INC_SYS
        ${MLCRS_THIRDPARTY_PATH}/libcurl/include
        ${MLCRS_THIRDPARTY_PATH}/WebRTC/include
        ${MLCRS_THIRDPARTY_PATH}/WebRTC/include/third_party/abseil-cpp
        # ${MLCRS_THIRDPARTY_PATH}/RTCH265/include
        # ${MLCRS_THIRDPARTY_PATH}/RTCH265/include/third_party/abseil-cpp
        ${MLCRS_THIRDPARTY_PATH}/zlib/include
    )

    set(SRC
        Private/AudioSink.cpp
        Private/AudioSink.h
        Private/AudioDeviceModule.cpp
        Private/AudioDeviceModule.h
        Private/DataChannelObserver.cpp
        Private/DataChannelObserver.h 
        Private/EncoderFactory.cpp
        Private/EncoderFactory.h 
        Private/FixedFPSPump.cpp
        Private/FixedFPSPump.h
        Private/FrameBuffer.cpp
        Private/FrameBuffer.h
        Private/IPixelStreamingAudioConsumer.h
        Private/IPixelStreamingAudioSink.h
        Private/IPixelStreamingSessions.h
        Private/PixelStreamingPlayerId.h
        Private/PixelStreamingStatNames.h
        Private/PixelStreamingStreamer.cpp
        Private/PlayerSession.cpp
        Private/PlayerSession.h
        Private/ProtocolDefs.h
        Private/SessionDescriptionObservers.h
        Private/SignallingServerConnection.cpp
        Private/SignallingServerConnection.h
        Private/SimulcastEncoderAdapter.cpp
        Private/SimulcastEncoderAdapter.h
        Private/Streamer.cpp
        Private/Streamer.h
        Private/TextureSource.cpp
        Private/TextureSource.h
        Private/ThreadSafePlayerSessions.cpp
        Private/ThreadSafePlayerSessions.h
        Private/UnrealReceiveThread.cpp
        Private/VideoDecoderFactory.cpp
        Private/VideoDecoderFactory.h
        Private/VideoEncoderH264Wrapper.cpp
        Private/VideoEncoderH264Wrapper.h
        Private/VideoEncoderRTC.cpp
        Private/VideoEncoderRTC.h
        Private/VideoSource.cpp
        Private/VideoSource.h
        Private/WebSocketClientImplCurl.cpp
        Private/WebSocketClientImplCurl.h
        
        Public/PixelStreamingStreamer.h
        Public/UnrealReceiveThread.h
    )

    set(LIB
        OpenZICore
        mlcrs_ipctool
        mlcrs_avencoder
        mlcrs_util
        mlcrs_config

        debug ${MLCRS_THIRDPARTY_PATH}/zlib/lib/Debug/zlibstaticd.lib
        debug ${MLCRS_THIRDPARTY_PATH}/libOpus/lib/Debug/speex_resampler.lib
        debug ${MLCRS_THIRDPARTY_PATH}/libOpus/lib/Debug/opus.lib
        debug ${MLCRS_THIRDPARTY_PATH}/libOpus/lib/Debug/opus_sse41.lib
        # debug ${MLCRS_THIRDPARTY_PATH}/WebRTC/lib/Debug/webrtc.lib
        # debug ${MLCRS_THIRDPARTY_PATH}/RTCH265/lib/Debug/webrtc.lib
        debug ${MLCRS_THIRDPARTY_PATH}/libcurl/lib/Debug/libcurld.lib
        debug ${MLCRS_THIRDPARTY_PATH}/OpenSSL/lib/Debug/libcrypto.lib
        debug ${MLCRS_THIRDPARTY_PATH}/OpenSSL/lib/Debug/libssl.lib

        optimized ${MLCRS_THIRDPARTY_PATH}/zlib/lib/Release/zlibstatic.lib
        optimized ${MLCRS_THIRDPARTY_PATH}/libOpus/lib/Release/speex_resampler.lib
        optimized ${MLCRS_THIRDPARTY_PATH}/libOpus/lib/Release/opus.lib
        optimized ${MLCRS_THIRDPARTY_PATH}/libOpus/lib/Release/opus_sse41.lib
        optimized ${MLCRS_THIRDPARTY_PATH}/WebRTC/lib/Release/webrtc.lib
        # optimized ${MLCRS_THIRDPARTY_PATH}/RTCH265/lib/Release/webrtc.lib
        optimized ${MLCRS_THIRDPARTY_PATH}/libcurl/lib/Release/libcurl.lib
        optimized ${MLCRS_THIRDPARTY_PATH}/OpenSSL/lib/Release/libcrypto.lib
        optimized ${MLCRS_THIRDPARTY_PATH}/OpenSSL/lib/Release/libssl.lib
        Secur32.lib
        Ws2_32.lib
        Wldap32.lib
        crypt32.lib
        iphlpapi.lib
    )

    set(PRIVATELIB

    )

    set(_WARNINGS
    # warning level:
    "/W3"
    # disable:
    "/wd4005"
    )

    string(REPLACE ";" " " _WARNINGS "${_WARNINGS}")
    set(C_WARNINGS "${_WARNINGS}")
    set(CXX_WARNINGS "${_WARNINGS}")
    unset(_WARNINGS)

    set(CMAKE_C_FLAGS "${C_WARNINGS} ${CMAKE_C_FLAGS}")
    set(CMAKE_CXX_FLAGS "${CXX_WARNINGS} ${CMAKE_CXX_FLAGS}")


    mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
    set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
        CXX_STANDARD 20
        CXX_EXTENSIONS OFF
        CXX_STANDARD_REQUIRED ON
        POSITION_INDEPENDENT_CODE 1
    )
    # target_link_libraries(mlcrs_pixelstreaming INTERFACE ${PRIVATELIB})
elseif(CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DWEBRTC_LINUX=1)
    add_definitions(-DWEBRTC_POSIX=1)
    add_definitions(-DWEBRTC_VERSION=84)
    # add_definitions(-DABSL_ALLOCATOR_NOTHROW=1)
    add_definitions(-DCURL_STATICLIB -DBUILDING_LIBCURL -DUSE_IPV6)
    add_definitions(-DOpenZI_DUMP_TEXTURE_PROCESSTIME=0)

    find_package(ZLIB REQUIRED)
    if (NOT ZLIB_FOUND)
        message(STATUS "ZLIB Not Found")
    else()
        # message(STATUS ${ZLIB_INCLUDE_DIRS})
        # message(STATUS ${ZLIB_LIBRARIES})
    endif()

    find_package(OpenSSL REQUIRED)
    if (NOT OPENSSL_FOUND)
        message(STATUS "OpenSSL Not Found")
    else()
        # message(STATUS ${OPENSSL_INCLUDE_DIRS})
        # message(STATUS ${OPENSSL_LIBRARIES})
    endif()

    find_package(CURL REQUIRED)
    if (NOT CURL_FOUND)
        message(STATUS "CURL Not Found")
    else()
        # message(STATUS ${CURL_INCLUDE_DIRS})
        # message(STATUS ${CURL_LIBRARIES})
    endif()

    set(INC
        .
        Private
        Public
        ../Core/Public
        ../Core/Public/Containers
        ../Core/Public/MessageCenter
        ../AVEncoder/Public
        ../Config/Public
        ../Util/Public
        ../IPCTool/Public
        ../RHI/Public
        ../D3D11RHI/Public
        ../D3D12RHI/Public
        ../VulkanRHI/Public
        # ${MLCRS_THIRDPARTY_PATH}/OpenSSL/Linux/include/openssl
        # ${MLCRS_THIRDPARTY_PATH}/OpenSSL/Linux/include
    )

    set(INC_SYS
        ${MLCRS_THIRDPARTY_PATH}/zlib/Linux/include
        ${MLCRS_THIRDPARTY_PATH}/libOpus/Linux/include
        ${MLCRS_THIRDPARTY_PATH}/libcurl/Linux/include
        ${MLCRS_THIRDPARTY_PATH}/WebRTC/include
        ${MLCRS_THIRDPARTY_PATH}/WebRTC/include/third_party/abseil-cpp
    )

    set(SRC
        Private/AudioSink.cpp
        Private/AudioSink.h
        Private/AudioDeviceModule.cpp
        Private/AudioDeviceModule.h
        Private/DataChannelObserver.cpp
        Private/DataChannelObserver.h 
        Private/EncoderFactory.cpp
        Private/EncoderFactory.h 
        Private/FixedFPSPump.cpp
        Private/FixedFPSPump.h
        Private/FrameBuffer.cpp
        Private/FrameBuffer.h
        Private/IPixelStreamingAudioConsumer.h
        Private/IPixelStreamingAudioSink.h
        Private/IPixelStreamingSessions.h
        Private/PixelStreamingPlayerId.h
        Private/PixelStreamingStatNames.h
        Private/PixelStreamingStreamer.cpp
        Private/PlayerSession.cpp
        Private/PlayerSession.h
        Private/ProtocolDefs.h
        Private/SessionDescriptionObservers.h
        Private/SignallingServerConnection.cpp
        Private/SignallingServerConnection.h
        Private/SimulcastEncoderAdapter.cpp
        Private/SimulcastEncoderAdapter.h
        Private/Streamer.cpp
        Private/Streamer.h
        Private/TextureSource.cpp
        Private/TextureSource.h
        Private/ThreadSafePlayerSessions.cpp
        Private/ThreadSafePlayerSessions.h
        Private/UnrealReceiveThread.cpp
        Private/VideoEncoderH264Wrapper.cpp
        Private/VideoEncoderH264Wrapper.h
        Private/VideoEncoderRTC.cpp
        Private/VideoEncoderRTC.h
        Private/VideoSource.cpp
        Private/VideoSource.h
        Private/WebSocketClientImplCurl.cpp
        Private/WebSocketClientImplCurl.h
        Private/FdSocket.cpp
        
        Public/PixelStreamingStreamer.h
        Public/UnrealReceiveThread.h
        Public/FdSocket.h
    )

# add_definitions(-DPICOJSON_USE_INT64)
    set(LIB
        # OpenZICore
        # mlcrs_config
        mlcrs_util
        mlcrs_ipctool
        mlcrs_avencoder

        # debug ${MLCRS_THIRDPARTY_PATH}/zlib/Linux/lib/libz_fPIC.a
        debug ${MLCRS_THIRDPARTY_PATH}/libOpus/Linux/lib/libresampler.a
        debug ${MLCRS_THIRDPARTY_PATH}/libOpus/Linux/lib/libopus.a
        # debug ${MLCRS_THIRDPARTY_PATH}/OpenSSL/Linux/lib/libssl.a
        # debug ${MLCRS_THIRDPARTY_PATH}/OpenSSL/Linux/lib/libcrypto.a
        # debug ${MLCRS_THIRDPARTY_PATH}/libcurl/Linux/lib/libcurl.a

        # optimized ${MLCRS_THIRDPARTY_PATH}/zlib/Linux/lib/libz_fPIC.a
        optimized ${MLCRS_THIRDPARTY_PATH}/libOpus/Linux/lib/libresampler.a
        optimized ${MLCRS_THIRDPARTY_PATH}/libOpus/Linux/lib/libopus.a
        # optimized ${MLCRS_THIRDPARTY_PATH}/OpenSSL/Linux/lib/libssl.a
        # optimized ${MLCRS_THIRDPARTY_PATH}/OpenSSL/Linux/lib/libcrypto.a
        # optimized ${MLCRS_THIRDPARTY_PATH}/libcurl/Linux/lib/libcurl.a
        # optimized ${MLCRS_THIRDPARTY_PATH}/WebRTC/lib/Release/libwebrtc.a
        # ${ZLIB_LIBRARIES}
        # ${OPENSSL_LIBRARIES}
        ${CURL_LIBRARIES}
        X11
        debug ${MLCRS_THIRDPARTY_PATH}/WebRTC/lib/Debug/libwebrtc.a
        optimized ${MLCRS_THIRDPARTY_PATH}/WebRTC/lib/Release/libwebrtc.a
    )

    mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")
endif()