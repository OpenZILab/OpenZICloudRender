# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/08/24 09:28
# 

cmake_minimum_required(VERSION 3.20)
project(OpenZICore LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(INC
    .
    Private
    Public
)

set(INC_SYS

)

set(SRC
    Public/CoreMinimal.h
)

set(LIB
)

option(BUILD_CONTAINERS "Whether to compile container modules to library files" ON)


if(BUILD_CONTAINERS)
    set(INC
        ${INC}
        Private/Containers
        Public/Containers
    )
    set(SRC
        ${SRC}
        Private/Containers/Queue.cpp

        Public/Containers/Queue.h
        Public/Containers/IntPoint.h
        Public/Containers/RefCounting.h
        Public/Containers/FormatString.h
    )
endif()

option(BUILD_MESSAGECENTER "Whether to compile message center module to library file" ON)

if (BUILD_MESSAGECENTER)
    set(INC
        ${INC}
        Private/MessageCenter
        Public/MessageCenter
    )
    set(SRC
        ${SRC}
        Private/MessageCenter/MessageCenter.cpp
        Public/MessageCenter/MessageCenter.h
    )

endif()

option(BUILD_THREAD "Whether to compile message thread module to library file" ON)

if (BUILD_THREAD)
    set(INC
        ${INC}
        Private/Thread
        Public/Thread
    )
    set(SRC
        ${SRC}
        Private/Thread/CriticalSection.cpp
        Private/Thread/Thread.cpp
        Public/Thread/CriticalSection.h
        Public/Thread/Thread.h
    )
endif()

option(BUILD_LOGGER "Whether to compile message logger module to library file" ON)

if (BUILD_LOGGER)
    set(INC
        ${INC}
        Private/Logger
        Public/Logger
    )
    set(SRC
        ${SRC}
        Private/Logger/CommandLogger.cpp
        Public/Logger/CommandLogger.h
    )
endif()

mlcrs_add_lib(${PROJECT_NAME} "${SRC}" "${INC}" "${INC_SYS}" "${LIB}")