# 
# Copyright by Cengzi Technology Co., Ltd
# Created by OpenZILab
# DateTime: 2022/07/26 16:56
# 

# Support per-target CMake flags
# Read from: CMAKE_C_FLAGS_**** (made upper case) when set.
#
# 'name' should always match the target name,
# use this macro before add_library or add_executable.
#
# Optionally takes an arg passed to set(), eg PARENT_SCOPE.
macro(add_cc_flags_custom_test
  name
  )

  string(TOUPPER ${name} _name_upper)
  if(DEFINED CMAKE_C_FLAGS_${_name_upper})
    message(STATUS "Using custom CFLAGS: CMAKE_C_FLAGS_${_name_upper} in \"${CMAKE_CURRENT_SOURCE_DIR}\"")
    string(APPEND CMAKE_C_FLAGS " ${CMAKE_C_FLAGS_${_name_upper}}" ${ARGV1})
  endif()
  if(DEFINED CMAKE_CXX_FLAGS_${_name_upper})
    message(STATUS "Using custom CXXFLAGS: CMAKE_CXX_FLAGS_${_name_upper} in \"${CMAKE_CURRENT_SOURCE_DIR}\"")
    string(APPEND CMAKE_CXX_FLAGS " ${CMAKE_CXX_FLAGS_${_name_upper}}" ${ARGV1})
  endif()
  unset(_name_upper)

endmacro()

macro(add_delayload_dlls
  name
  )

  set(dlls "${ARGN}")
  foreach(dll ${dlls})
    set(${name} "${${name}} /DELAYLOAD:${dll}.dll")
  endforeach()
endmacro()


# Nicer makefiles with -I/1/foo/ instead of -I/1/2/3/../../foo/
# use it instead of include_directories()
function(mlcrs_include_dirs
  includes
  )

  set(_ALL_INCS "")
  foreach(_INC ${ARGV})
    get_filename_component(_ABS_INC ${_INC} ABSOLUTE)
    list(APPEND _ALL_INCS ${_ABS_INC})
    # for checking for invalid includes, disable for regular use
    # if(NOT EXISTS "${_ABS_INC}/")
    #   message(FATAL_ERROR "Include not found: ${_ABS_INC}/")
    # endif()
  endforeach()
  include_directories(${_ALL_INCS})
endfunction()

function(mlcrs_include_dirs_sys
  includes
  )

  set(_ALL_INCS "")
  foreach(_INC ${ARGV})
    get_filename_component(_ABS_INC ${_INC} ABSOLUTE)
    list(APPEND _ALL_INCS ${_ABS_INC})
    # if(NOT EXISTS "${_ABS_INC}/")
    #   message(FATAL_ERROR "Include not found: ${_ABS_INC}/")
    # endif()
  endforeach()
  include_directories(SYSTEM ${_ALL_INCS})
endfunction()

function(mlcrs_add_lib_nolist
  name
  sources
  includes
  includes_sys
  library_deps
  )
  add_cc_flags_custom_test(${name} PARENT_SCOPE)

  mlcrs_add_lib__impl(${name} "${sources}" "${includes}" "${includes_sys}" "${library_deps}")
endfunction()

function(mlcrs_add_lib
  name
  sources
  includes
  includes_sys
  library_deps
  )
  add_cc_flags_custom_test(${name} PARENT_SCOPE)
  
  mlcrs_add_lib__impl(${name} "${sources}" "${includes}" "${includes_sys}" "${library_deps}")
  
  set_property(GLOBAL APPEND PROPERTY MLCRS_LINK_LIBS ${name})
endfunction()

# only MSVC uses SOURCE_GROUP
function(mlcrs_add_lib__impl
  name
  sources
  includes
  includes_sys
  library_deps
  )

  # message(STATUS "Configuring library ${name}")

  # include_directories(${includes})
  # include_directories(SYSTEM ${includes_sys})
  mlcrs_include_dirs("${includes}")
  mlcrs_include_dirs_sys("${includes_sys}")

  add_library(${name} STATIC ${sources})

  if (CMAKE_SYSTEM_NAME MATCHES "Windows")
    if (CMAKE_BUILD_TYPE MATCHES "Debug")
      if (BUILD_WITH_RUNTIME_CRT)
        target_compile_options(${name} PRIVATE "/MDd")
      else()
        target_compile_options(${name} PRIVATE "/MTd")
      endif()
    else()
      if (BUILD_WITH_RUNTIME_CRT)
        target_compile_options(${name} PRIVATE "/MD")
      else()
        target_compile_options(${name} PRIVATE "/MT")
      endif()
      target_compile_options(
        ${name}
        PRIVATE "/Zi"
      )
    endif()
  endif()

  # On Windows certain libraries have two sets of binaries: one for debug builds and one for
  # release builds. The root of this requirement goes into ABI, I believe, but that's outside
  # of a scope of this comment.
  #
  # CMake have a native way of dealing with this, which is specifying what build type the
  # libraries are provided for:
  #
  #   target_link_libraries(tagret optimized|debug|general <libraries>)
  #
  # The build type is to be provided as a separate argument to the function.
  #
  # CMake's variables for libraries will contain build type in such cases. For example:
  #
  #   set(FOO_LIBRARIES optimized libfoo.lib debug libfoo_d.lib)
  #
  # Complications starts with a single argument for library_deps: all the elements are being
  # put to a list: "${FOO_LIBRARIES}" will become "optimized;libfoo.lib;debug;libfoo_d.lib".
  # This makes it impossible to pass it as-is to target_link_libraries sine it will treat
  # this argument as a list of libraries to be linked against, causing missing libraries
  # for optimized.lib.
  #
  # What this code does it traverses library_deps and extracts information about whether
  # library is to provided as general, debug or optimized. This is a little state machine which
  # keeps track of which build type library is to provided for:
  #
  # - If "debug" or "optimized" word is found, the next element in the list is expected to be
  #   a library which will be passed to target_link_libraries() under corresponding build type.
  #
  # - If there is no "debug" or "optimized" used library is specified for all build types.
  #
  # NOTE: If separated libraries for debug and release are needed every library is the list are
  # to be prefixed explicitly.
  #
  #  Use: "optimized libfoo optimized libbar debug libfoo_d debug libbar_d"
  #  NOT: "optimized libfoo libbar debug libfoo_d libbar_d"
  if(NOT "${library_deps}" STREQUAL "")
    set(next_library_mode "")
    foreach(library ${library_deps})
      string(TOLOWER "${library}" library_lower)
      if(("${library_lower}" STREQUAL "optimized") OR
         ("${library_lower}" STREQUAL "debug"))
        set(next_library_mode "${library_lower}")
      else()
        if("${next_library_mode}" STREQUAL "optimized")
          target_link_libraries(${name} PUBLIC optimized ${library})
        elseif("${next_library_mode}" STREQUAL "debug")
          target_link_libraries(${name} PUBLIC debug ${library})
        else()
          target_link_libraries(${name} PUBLIC ${library})
        endif()
        set(next_library_mode "")
      endif()
    endforeach()
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
      target_link_libraries(${name} PUBLIC stdc++)
    endif()
    # INTERFACE: 对于INTERFACE关键字，使用的情形为：liball.so没有使用libsub.so，但是liball.so对外暴露libsub.so的接口，也就是liball.so的头文件包含了libsub.so的头文件，在其它目标使用liball.so的功能的时候，可能必须要使用libsub.so的功能
    # PUBLIC: 对于PUBLIC关键字（PUBLIC=PRIVATE+INTERFACE），使用的情形为：liball.so使用了libsub.so，并且liball.so对外暴露了libsub.so的接口
    # PRIVATE: 对于PRIVATE关键字，使用的情形为：liball.so使用了libsub.so，但是liball.so并不对外暴露libsub.so的接口
  endif()

  # works fine without having the includes
  # listed is helpful for IDE's (QtCreator/MSVC)
  mlcrs_source_group("${name}" "${sources}")
  mlcrs_user_header_search_paths("${name}" "${includes}")

  list_assert_duplicates("${sources}")
  list_assert_duplicates("${includes}")
  # Not for system includes because they can resolve to the same path
  # list_assert_duplicates("${includes_sys}")

endfunction()

function(list_assert_duplicates
  list_id
  )

  # message(STATUS "list data: ${list_id}")

  list(LENGTH list_id _len_before)
  list(REMOVE_DUPLICATES list_id)
  list(LENGTH list_id _len_after)
  # message(STATUS "list size ${_len_before} -> ${_len_after}")
  if(NOT _len_before EQUAL _len_after)
    message(FATAL_ERROR "duplicate found in list which should not contain duplicates: ${list_id}")
  endif()
  unset(_len_before)
  unset(_len_after)
endfunction()

# Set include paths for header files included with "*.h" syntax.
# This enables auto-complete suggestions for user header files on Xcode.
# Build process is not affected since the include paths are the same
# as in HEADER_SEARCH_PATHS.
function(mlcrs_user_header_search_paths
  name
  includes
  )

  if(XCODE)
    set(_ALL_INCS "")
    foreach(_INC ${includes})
      get_filename_component(_ABS_INC ${_INC} ABSOLUTE)
      # _ALL_INCS is a space-separated string of file paths in quotes.
      string(APPEND _ALL_INCS " \"${_ABS_INC}\"")
    endforeach()
    set_target_properties(${name} PROPERTIES XCODE_ATTRIBUTE_USER_HEADER_SEARCH_PATHS "${_ALL_INCS}")
  endif()
endfunction()

function(mlcrs_source_group
  name
  sources
  )

  # if enabled, use the sources directories as filters.
  if(IDE_GROUP_SOURCES_IN_FOLDERS)
    foreach(_SRC ${sources})
      # remove ../'s
      get_filename_component(_SRC_DIR ${_SRC} REALPATH)
      get_filename_component(_SRC_DIR ${_SRC_DIR} DIRECTORY)
      string(FIND ${_SRC_DIR} "${CMAKE_CURRENT_SOURCE_DIR}/" _pos)
      if(NOT _pos EQUAL -1)
        string(REPLACE "${CMAKE_CURRENT_SOURCE_DIR}/" "" GROUP_ID ${_SRC_DIR})
        string(REPLACE "/" "\\" GROUP_ID ${GROUP_ID})
        source_group("${GROUP_ID}" FILES ${_SRC})
      endif()
      unset(_pos)
    endforeach()
  else()
    # Group by location on disk
    source_group("Source Files" FILES CMakeLists.txt)
    foreach(_SRC ${sources})
      get_filename_component(_SRC_EXT ${_SRC} EXT)
      if((${_SRC_EXT} MATCHES ".h") OR
         (${_SRC_EXT} MATCHES ".hpp") OR
         (${_SRC_EXT} MATCHES ".hh"))

        set(GROUP_ID "Header Files")
      elseif(${_SRC_EXT} MATCHES ".glsl$")
        set(GROUP_ID "Shaders")
      else()
        set(GROUP_ID "Source Files")
      endif()
      source_group("${GROUP_ID}" FILES ${_SRC})
    endforeach()
  endif()

  # if enabled, set the FOLDER property for the projects
  if(IDE_GROUP_PROJECTS_IN_FOLDERS)
    get_filename_component(FolderDir ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)
    string(REPLACE ${CMAKE_SOURCE_DIR} "" FolderDir ${FolderDir})
    set_target_properties(${name} PROPERTIES FOLDER ${FolderDir})
  endif()
endfunction()