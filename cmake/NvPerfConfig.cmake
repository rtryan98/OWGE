#[[
* Copyright 2014-2023 NVIDIA Corporation.  All rights reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.

Try to find NvPerfSDK.

This will define:
  NvPerf_FOUND          - successfully found all 64bit components
  NvPerf_INCLUDE_DIRS   - paths to NvPerf headers directories
  NvPerf_LIBRARY_DIRS   - paths to NvPerf .lib/.dll or .so
  NvPerf_LIBRARIES      - includes NvPerf Host library.

This will export the following targets (suitable for static linking):
  NvPerf                       - NvPerf include directories only - intended for runtime loading of .dll/.so with nvperf_host_impl.h or nvperf_target_impl.h
  NvPerf-shared                - NvPerf Host and Target library - defines include directories

This will export the following functions:
  NvPerfDeploy         - Deploy NvPerf .dll/.so to a targets deploy directory

It is recommended to use NvPerf to runtime load the .dll/.so (nvperf_host_impl.h) to reduce binary bloat when NvPerf is not enabled.

Can set the following cmake variables to provide a hint as to where to search for the NvPerfSDK:
  NVPERF_SDK_PATH

If the above cmake variables are not set, this will check them as environment variables as well.
]]

# Search order by priority

set(_NVPERF_SDK_PATH $ENV{NVPERF_SDK_PATH})
if(NVPERF_SDK_PATH)
  set(_NVPERF_SDK_PATH ${NVPERF_SDK_PATH})
endif()

set(NvPerf_search_paths
    ${_NVPERF_SDK_PATH}
    ${_NVPERF_SDK_PATH}/NvPerf
    ${CMAKE_CURRENT_LIST_DIR}/../NvPerf
    ${CMAKE_CURRENT_LIST_DIR}/../../NvPerf
)
find_path(NvPerf_INCLUDE_DIR "nvperf_host.h" HINTS ${NvPerf_search_paths} PATH_SUFFIXES "include" )

if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    message(FATAL_ERROR "32 bit is not supported.")
endif()

if (WIN32)
    find_path(NvPerf_OS_INCLUDE_DIR "nvperf_host_impl.h" HINTS ${NvPerf_search_paths} PATH_SUFFIXES "include/windows-desktop-x64")
    find_library(NvPerfHost_LIBRARY NAMES nvperf_host nvperf_grfx_host   HINTS ${NvPerf_search_paths} PATH_SUFFIXES "bin/x64")
else()
    get_filename_component(COMPILER_REAL_PATH ${CMAKE_C_COMPILER} REALPATH)
    if(${COMPILER_REAL_PATH} MATCHES "aarch64")
        find_path(NvPerf_OS_INCLUDE_DIR "nvperf_host_impl.h" HINTS ${NvPerf_search_paths} PATH_SUFFIXES "include/linux-l4t-a64")
        find_library(NvPerfHost_LIBRARY NAMES nvperf_host nvperf_grfx_host   HINTS ${NvPerf_search_paths} PATH_SUFFIXES "lib/a64")
    else()
        find_path(NvPerf_OS_INCLUDE_DIR "nvperf_host_impl.h" HINTS ${NvPerf_search_paths} PATH_SUFFIXES "include/linux-desktop-x64")
        find_library(NvPerfHost_LIBRARY NAMES nvperf_host nvperf_grfx_host   HINTS ${NvPerf_search_paths} PATH_SUFFIXES "lib/x64")
    endif()
endif()

set(NvPerf_FOUND TRUE)
foreach(_VAR NvPerf_INCLUDE_DIR NvPerf_OS_INCLUDE_DIR NvPerfHost_LIBRARY)
    if (NOT ${_VAR})
        message(STATUS "NvPerf - couldn't set ${_VAR}")
        set(NvPerf_FOUND FALSE)
    endif()
endforeach()

if (NvPerf_FOUND)
    message(STATUS "Found NvPerf")
    set(NvPerf_INCLUDE_DIRS ${NvPerf_INCLUDE_DIR} ${NvPerf_OS_INCLUDE_DIR})

    get_filename_component(NvPerf_LIBRARY_DIR ${NvPerfHost_LIBRARY} DIRECTORY)
    set(NvPerf_LIBRARY_DIRS ${NvPerf_LIBRARY_DIR})

    set(NvPerf_LIBRARIES ${NvPerfHost_LIBRARY})
    
    macro(_SetSharedLib)
        if (WIN32)
            get_filename_component(NvPerf_SHARED_LIBRARY_PATH ${ARGV1} DIRECTORY)
            get_filename_component(NvPerf_SHARED_LIBRARY_NAME_WE ${ARGV1} NAME_WE)
            set(NvPerf_SHARED_LIBRARY "${NvPerf_SHARED_LIBRARY_PATH}/${NvPerf_SHARED_LIBRARY_NAME_WE}.dll")
        else()
            set(NvPerf_SHARED_LIBRARY ${ARGV1})
        endif()
        set_property(TARGET ${ARGV0} PROPERTY INTERFACE_NVPERF_PROPERTY_DEPLOY_LIB ${NvPerf_SHARED_LIBRARY})
        set_property(TARGET ${ARGV0} APPEND PROPERTY COMPATIBLE_INTERFACE_STRING NVPERF_PROPERTY_DEPLOY_LIB)
    endmacro()

    # Target NvPerf
    add_library(NvPerf INTERFACE)
    _SetSharedLib(NvPerf ${NvPerfHost_LIBRARY})
    set_target_properties(NvPerf PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${NvPerf_INCLUDE_DIRS}")
    if (NOT WIN32)
        set_target_properties(NvPerf PROPERTIES INTERFACE_LINK_LIBRARIES dl)
    endif()

    # Target NvPerf-shared
    add_library(NvPerf-shared SHARED IMPORTED)
    _SetSharedLib(NvPerf-shared  ${NvPerfHost_LIBRARY})
    set_target_properties(NvPerf-shared PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${NvPerf_INCLUDE_DIRS}")
    if (WIN32)
        set_target_properties(NvPerf-shared PROPERTIES IMPORTED_IMPLIB  ${NvPerfHost_LIBRARY})
    else ()
        set_target_properties(NvPerf-shared PROPERTIES INTERFACE_LINK_LIBRARIES dl)
    endif()
    set_target_properties(NvPerf-shared PROPERTIES IMPORTED_LOCATION ${NvPerf_SHARED_LIBRARY})

    # Copy NvPerf .dll/.so to target deploy directory
    #   deployTarget - target that includes NvPerf
    #   nvPerfTarget - as defined above
    function(DeployNvPerf deployTarget nvPerfTarget)
        get_target_property(NvPerf_DEPLOY_LIBRARY ${nvPerfTarget} INTERFACE_NVPERF_PROPERTY_DEPLOY_LIB)
        if (WIN32)
            add_custom_command(TARGET ${deployTarget} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${NvPerf_DEPLOY_LIBRARY} $<TARGET_FILE_DIR:${deployTarget}>
            )
        else()
            add_custom_command(TARGET ${deployTarget} POST_BUILD
                COMMAND cp -P ${NvPerf_DEPLOY_LIBRARY}* $<TARGET_FILE_DIR:${deployTarget}>
            )
        endif()
    endfunction()
else()
    message(STATUS
        " Unable to find NvPerf.  Please set environment variable:\n"
        "   NVPERF_SDK_PATH\n"
        " to the NvPerf install path or copy the NvPerf SDK to one of:\n"
        "   <NvPerf Samples Root>\n"
        "   <NvPerf Samples Root>/..\n")
endif(NvPerf_FOUND)
