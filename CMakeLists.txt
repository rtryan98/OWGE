cmake_minimum_required(VERSION 3.18)
project(OWGE CXX)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "" FORCE) # remove MinSizeRel

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/NvPerfConfig.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/NvPerfUtilityConfig.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/owge_general.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/owge_shaders.cmake)

add_owge_shader_lib(owge_shaders)

# Dependencies.
download_extract(
    https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2212.1/dxc_2023_03_01.zip
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    dxc_2023_03_01
)

copy_dll_if_not_exist(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dxc_2023_03_01/bin/x64/
    .
)

download_extract(
    https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.610.4
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    d3d12_agility_1.610.4
)

copy_dll_if_not_exist(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.610.4/build/native/bin/x64/
    D3D12
)

download_extract(
    https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.230302001
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    win_pix_event_runtime
)

copy_dll_if_not_exist(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/win_pix_event_runtime/bin/x64/
    .
)

download_extract(
    https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DirectStorage/1.2.0
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    direct_storage_1.2.0
)

copy_dll_if_not_exist(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/direct_storage_1.2.0/native/bin/x64/
    .
)

add_subdirectory(thirdparty)

# Options.
option(OWGE_USE_NVPERF "Use NVIDIA Nsight Perf SDK" OFF)
option(OWGE_USE_WIN_PIX_EVENT_RUNTIME "Use WinPixEventRuntime" ON)

# Project.
add_owge_lib(owge_common)

add_owge_lib(owge_d3d12_base)
target_include_directories(
    owge_d3d12_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.610.4/build/native
)
target_link_libraries(
    owge_d3d12_base PUBLIC
    d3d12.lib
    dxgi.lib
)

add_owge_lib(owge_asset)
target_include_directories(
    owge_d3d12_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.610.4/build/native
)
target_link_libraries(
    owge_d3d12_base PUBLIC
    owge_common
)

add_owge_lib(owge_window)

add_owge_lib(owge_render_engine)
target_include_directories(
    owge_render_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/direct_storage_1.2.0/native/include
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dxc_2023_03_01/inc
)
target_link_libraries(
    owge_render_engine PUBLIC
    owge_common
    owge_d3d12_base
    owge_window
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/direct_storage_1.2.0/native/lib/x64/dstorage.lib
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dxc_2023_03_01/lib/x64/dxcompiler.lib
    unordered_dense
)

add_owge_lib(owge_imgui)
target_include_directories(
    owge_imgui PUBLIC
)
target_link_libraries(
    owge_imgui PUBLIC
    owge_render_engine
    imgui
    implot
)

add_owge_lib(owge_render_techniques)
target_link_libraries(
    owge_render_techniques PUBLIC
    owge_render_engine
    owge_imgui
    owge_asset
)

add_owge_exe(owge_tech_demo)
target_link_libraries(
    owge_tech_demo PUBLIC
    owge_render_engine
    owge_render_techniques
)
target_include_directories(
    owge_tech_demo PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/tclap/include
)
add_dependencies(owge_tech_demo owge_shaders)

if(OWGE_USE_NVPERF)
    message(STATUS "Building owge_render_engine with NvPerf.")
    if(NOT NvPerf_FOUND)
        message(FATAL_ERROR
            "NVIDIA Nsight Perf SDK was not found and "
            "however OWGE_USE_NVPERF was set to true.")
    endif()

    target_compile_definitions(
        owge_render_engine PUBLIC
        OWGE_USE_NVPERF=1)
    target_include_directories(
        owge_render_engine PUBLIC
        ${NvPerf_INCLUDE_DIRS}
        ${NvPerfUtility_INCLUDE_DIRS}
    )
    target_link_libraries(
        owge_render_engine PUBLIC
        ${NvPerf_LIBRARIES}
        ${NvPerfUtility_LIBRARIES}
    )
    deploynvperf(owge_tech_demo NvPerf-shared)
endif()

if(OWGE_USE_WIN_PIX_EVENT_RUNTIME)
    message(STATUS "Building owge_render_engine with WinPixEventRuntime.")
    target_compile_definitions(
        owge_render_engine PUBLIC
        OWGE_USE_WIN_PIX_EVENT_RUNTIME=1
        USE_PIX)
    target_include_directories(
        owge_render_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/win_pix_event_runtime/Include/
        )
    target_link_libraries(
        owge_render_engine PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/win_pix_event_runtime/bin/x64/WinPixEventRuntime.lib
    )
endif()

if(MSVC)
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT owge_tech_demo)
    set_source_files_properties( # Why can't I declare that where it belongs CMake? Why does it have to be HERE??
        ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui/backends/imgui_impl_dx12.cpp
        PROPERTIES COMPILE_OPTIONS "/W2"
    )
endif()
