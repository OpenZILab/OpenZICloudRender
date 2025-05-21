# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/07/26 16:52
# 

# Allow selection of common build types via UI
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE
      "Release"
      CACHE STRING
            "MLCRS build type [Release, Debug]" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Release Debug)
endif()

set(CMAKE_BUILD_RPATH_USE_ORIGIN ON)

cmake_minimum_required(VERSION 3.20.0)
project(MLCRS VERSION 0.1.0)

option(BUILD_WITH_RUNTIME_CRT "use MD/MDd if true" ON)

#-----------------------------------------------------------------------------
# Load some macros.
include(build_files/cmake/macros.cmake)
#-----------------------------------------------------------------------------
# CPack for generating packages
include(build_files/cmake/packaging.cmake)


# Select C++17 as the standard for C++ projects.
set(CMAKE_CXX_STANDARD 17)
# If C++17 is not available, downgrading to an earlier standard is NOT OK.
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Do not enable compiler specific language extensions.
set(CMAKE_CXX_EXTENSIONS OFF)

add_definitions(-DNOMINMAX)
add_definitions(-D_WINSOCKAPI_)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
    add_definitions(-DPLATFORM_LINUX=1)
    add_definitions(-DPLATFORM_WINDOWS=0)
elseif (CMAKE_SYSTEM_NAME MATCHES "Windows")
    add_definitions(-DPLATFORM_LINUX=0)
    add_definitions(-DPLATFORM_WINDOWS=1)
    add_definitions(-DWEBRTC_WIN=1)
endif()

set(MLCRS_THIRDPARTY_PATH "${CMAKE_SOURCE_DIR}/thirdparty")
set(MLCRS_SOURCE_PATH "${CMAKE_SOURCE_DIR}/source")

option(WITH_OPENMP        "Enable OpenMP (has to be supported by the compiler)" ON)
#-----------------------------------------------------------------------------
# Configure OpenMP.
if(WITH_OPENMP)
  if(NOT OPENMP_CUSTOM)
    find_package(OpenMP)
  endif()

  if(OPENMP_FOUND)
    if(NOT WITH_OPENMP_STATIC)
      string(APPEND CMAKE_C_FLAGS " ${OpenMP_C_FLAGS}")
      string(APPEND CMAKE_CXX_FLAGS " ${OpenMP_CXX_FLAGS}")
      string(APPEND CMAKE_EXE_LINKER_FLAGS " ${OpenMP_LINKER_FLAGS}")
      string(APPEND CMAKE_MODULE_LINKER_FLAGS " ${OpenMP_LINKER_FLAGS}")
    else()
      # Typically avoid adding flags as defines but we can't
      # pass OpenMP flags to the linker for static builds, meaning
      # we can't add any OpenMP related flags to CFLAGS variables
      # since they're passed to the linker as well.
      add_definitions("${OpenMP_C_FLAGS}")

      find_library_static(OpenMP_LIBRARIES gomp ${CMAKE_CXX_IMPLICIT_LINK_DIRECTORIES})
    endif()
  else()
    message(STATUS "OpenMP not found, disabling WITH_OPENMP")
    set(WITH_OPENMP OFF)
  endif()

  mark_as_advanced(
    OpenMP_C_FLAGS
    OpenMP_CXX_FLAGS
  )
endif()

# # most msvc warnings are C & C++
if (CMAKE_SYSTEM_NAME MATCHES "Windows")
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
endif()


# Make MSVC properly report the value of the __cplusplus preprocessor macro
# Available MSVC 15.7 (1914) and up, without this it reports 199711L regardless
# of the C++ standard chosen above.
if(MSVC AND MSVC_VERSION GREATER 1913)
string(APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus")
# /VERBOSE:LIB 
string(APPEND CMAKE_EXE_LINKER_FLAGS "/NODEFAULTLIB:LIBCMT")
# string(APPEND CMAKE_EXE_LINKER_FLAGS "/NODEFAULTLIB:MSVCRT")
  # 防止UTF-8格式文件的中文注释编译时报文件编码警告（中文在UTF-8 With BOM下不会警告）
  add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
  add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif()

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
  add_compile_options(-w)
endif()

# MLCRS internal features
option(WITH_MLCRS "Build mlcrs" ON)
mark_as_advanced(WITH_MLCRS)

# MLCRS compile options
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

#-----------------------------------------------------------------------------
# Libraries

if(WITH_MLCRS)
  # source after intern and extern to gather all
  # internal and external library information first, for test linking
  add_subdirectory(source)
  add_subdirectory(thirdparty)
endif()


set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
