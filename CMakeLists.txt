cmake_minimum_required(VERSION 3.18)
project(OWGE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "" FORCE) # remove MinSizeRel

include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/NvPerfConfig.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/NvPerfUtilityConfig.cmake)

# Helpers.
macro(download_extract URL DST_FOLDER FOLDER_NAME)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip)
        message(STATUS "Downloading ${URL} and unpacking to ${DST_FOLDER}/${FOLDER_NAME} .")
        file(
            DOWNLOAD
            ${URL}
            ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip
        )
    else()
        message(STATUS "${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip already exists. No download required.")
    endif()
    if(NOT EXISTS ${DST_FOLDER}/${FOLDER_NAME})
        message(STATUS "Extracting ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip to ${DST_FOLDER}/${FOLDER_NAME} .")
        file(
            ARCHIVE_EXTRACT
            INPUT ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip
            DESTINATION ${DST_FOLDER}/${FOLDER_NAME}
        )
    else()
        message(STATUS "${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip is already extracted to ${DST_FOLDER}/${FOLDER_NAME} .")
    endif()
endmacro()

macro(copy_if_not_exist SRC DST)
    if(NOT EXISTS ${DST}) # Should check for contents in case of directory.
        message(STATUS "Copying ${SRC} to ${DST} .")
        file(COPY ${SRC} DESTINATION ${DST})
    else()
        message(STATUS "${SRC} already exists at ${DST} .")
    endif()
endmacro()

macro(copy_dll_if_not_exist SRC DST)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        copy_if_not_exist(${SRC} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUTCONFIG}/${DST})
    endforeach()
endmacro()

macro(set_target_default_properties TARGET)
    set_target_properties(${TARGET} PROPERTIES CXX_STANDARD 23)
    if(MSVC)
        target_compile_options(
            ${TARGET} PRIVATE
            "/wd26812" # The enum type 'type-name' is unscoped. Prefer 'enum class' over 'enum'
            "/WX"
            "/W4"
            "/MP"
        )
        set_target_properties(
            ${TARGET} PROPERTIES
            VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endmacro()

macro(group_file_tree FILES DUPE_FILTER)
    foreach(ITEM IN ITEMS ${FILES})
        get_filename_component(SRC_PATH "${ITEM}" PATH)
        string(REPLACE "${CMAKE_SOURCE_DIR}" "" GROUP_PATH "${SRC_PATH}")
        string(REPLACE "${DUPE_FILTER}/${DUPE_FILTER}" "${DUPE_FILTER}" GROUP_PATH "${GROUP_PATH}")
        string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
        source_group("${GROUP_PATH}" FILES ${ITEM})
    endforeach()
endmacro()

macro(target_group_file_tree TARGET)
    get_target_property(${TARGET}_SOURCES ${TARGET} SOURCES)
    group_file_tree("${${TARGET}_SOURCES}" "${TARGET}")
endmacro()

macro(setup_owge_target TARGET)
    set_target_default_properties(${TARGET})
    add_subdirectory(${TARGET})
    target_group_file_tree(${TARGET})
    target_include_directories(
        ${TARGET} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}
    )
endmacro()

macro(add_owge_lib TARGET)
    add_library(${TARGET})
    setup_owge_target(${TARGET})
endmacro()

macro(add_owge_exe TARGET)
    add_executable(${TARGET})
    setup_owge_target(${TARGET})
endmacro()

# Shader compilation.
add_custom_target(owge_shaders)
set(DXC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dxc_2023_03_01/bin/x64/dxc.exe") # Change path depending on OS.
set(SHADER_COMPILE_PARAMS -HV 2021 -Zpr -no-legacy-cbuf-layout -enable-16bit-types -I "${CMAKE_CURRENT_SOURCE_DIR}/owge_shaders/")
macro(compile_hlsl SHADER MODEL ENTRYPOINT OUTFILE)
    string(REPLACE ".hlsl" ".json" SHADER_PERMUTATIONS ${SHADER})
    if(EXISTS ${SHADER_PERMUTATIONS})
        file(READ ${SHADER_PERMUTATIONS} SHADER_PERMUTATIONS)
        set(SHADER_COMPILE_COMMANDS)
        set(SHADER_COMPILE_OUTFILES)
        string(JSON SHADER_PERMUTATIONS GET ${SHADER_PERMUTATIONS} "permutations")
        string(JSON SHADER_PERMUHTATION_COUNT LENGTH ${SHADER_PERMUTATIONS})
        foreach(SHADER_PERMUTATION_ITER RANGE ${SHADER_PERMUHTATION_COUNT})
            set(SHADER_PERMUTATION_DEFINE_LIST)
            if(${SHADER_PERMUTATION_ITER} EQUAL ${SHADER_PERMUHTATION_COUNT})
                break()
            endif() # CMake includes the last element, we don't want that however.
            string(JSON SHADER_PERMUTATION GET ${SHADER_PERMUTATIONS} ${SHADER_PERMUTATION_ITER})
            string(JSON SHADER_PERMUTATION_DEFINES GET ${SHADER_PERMUTATION} "defines")
            string(JSON SHADER_PERMUTATION_DEFINE_COUNT LENGTH ${SHADER_PERMUTATION_DEFINES})
            string(JSON SHADER_PERMUTATION_NAME GET ${SHADER_PERMUTATION} "name")
            foreach(SHADER_PERMUTATION_DEFINE_ITER RANGE 0 ${SHADER_PERMUTATION_DEFINE_COUNT})
                if(${SHADER_PERMUTATION_DEFINE_ITER} EQUAL ${SHADER_PERMUTATION_DEFINE_COUNT})
                    break()
                endif()
                string(JSON SHADER_PERMUTATION_DEFINE GET ${SHADER_PERMUTATION_DEFINES} ${SHADER_PERMUTATION_DEFINE_ITER})
                string(JSON SHADER_PERMUTATION_DEFINE_NAME MEMBER ${SHADER_PERMUTATION_DEFINE} 0)
                string(JSON SHADER_PERMUTATION_DEFINE_VALUE GET ${SHADER_PERMUTATION_DEFINE} ${SHADER_PERMUTATION_DEFINE_NAME})
                set(SHADER_PERMUTATION_DEFINE_LIST ${SHADER_PERMUTATION_DEFINE_LIST} -D \ ${SHADER_PERMUTATION_DEFINE_NAME}=${SHADER_PERMUTATION_DEFINE_VALUE})
            endforeach()
            get_filename_component(SHADER_PERMUTATION_OUTFILE ${OUTFILE} NAME_WE)
            get_filename_component(SHADER_PERMUTATION_EXT ${OUTFILE} EXT)
            get_filename_component(SHADER_PERMUTATION_DIR ${OUTFILE} PATH)
            string(APPEND SHADER_PERMUTATION_OUTFILE "_${SHADER_PERMUTATION_NAME}")
            string(APPEND SHADER_PERMUTATION_OUTFILE "${SHADER_PERMUTATION_EXT}")
            string(PREPEND SHADER_PERMUTATION_OUTFILE "${SHADER_PERMUTATION_DIR}/")
            set(SHADER_COMPILE_OUTFILES ${SHADER_COMPILE_OUTFILES} "${SHADER_PERMUTATION_OUTFILE} ")
            set(SHADER_COMPILE_COMMANDS ${SHADER_COMPILE_COMMANDS} ${DXC_PATH} -T ${MODEL} -E ${ENTRYPOINT} ${SHADER_PERMUTATION_DEFINE_LIST} ${SHADER_COMPILE_PARAMS} -Fo ${SHADER_PERMUTATION_OUTFILE} ${SHADER} && )
        endforeach()
        set(SHADER_COMPILE_COMMANDS ${SHADER_COMPILE_COMMANDS} echo on) # Hacky but gets the job done. No string post processing which confuses CMake.
        add_custom_command(
            OUTPUT ${SHADER_COMPILE_OUTFILES}
            COMMAND ${SHADER_COMPILE_COMMANDS}
            MAIN_DEPENDENCY ${SHADER}
            COMMENT "Compiling ${SHADER} with permutations."
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
    else()
        add_custom_command(
            OUTPUT ${OUTFILE}
            COMMAND ${DXC_PATH} -T ${MODEL} -E ${ENTRYPOINT} ${SHADER_COMPILE_PARAMS} -Fo ${OUTFILE} ${SHADER}
            MAIN_DEPENDENCY ${SHADER}
            COMMENT "Compiling ${SHADER}."
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            VERBATIM
        )
    endif()
endmacro()

macro(compile_hlsl_profile SHADER PROFILE)
    string(REPLACE ".hlsl" ".bin" COMPILE_SHADER_OUT ${SHADER})
    if(${SHADER} MATCHES "owge_shaders/owge_shaders")
        string(REPLACE "owge_shaders/owge_shaders" "res/builtin/shader" COMPILE_SHADER_OUT ${COMPILE_SHADER_OUT})
    endif()
    compile_hlsl(${SHADER} ${PROFILE}_6_6 "${PROFILE}_main" ${COMPILE_SHADER_OUT})
endmacro()

add_subdirectory(owge_shaders)
target_group_file_tree(owge_shaders)
get_target_property(owge_shaders_SOURCES owge_shaders SOURCES)
foreach(ITEM IN ITEMS ${owge_shaders_SOURCES})
    if(${ITEM} MATCHES "\.vs\.hlsl")
        compile_hlsl_profile(${ITEM} vs)
    elseif(${ITEM} MATCHES "\.ps\.hlsl")
        compile_hlsl_profile(${ITEM} ps)
    elseif(${ITEM} MATCHES "\.cs\.hlsl")
        compile_hlsl_profile(${ITEM} cs)
    elseif(${ITEM} MATCHES "\.gs\.hlsl")
        compile_hlsl_profile(${ITEM} gs)
    elseif(${ITEM} MATCHES "\.ds\.hlsl")
        compile_hlsl_profile(${ITEM} ds)
    elseif(${ITEM} MATCHES "\.hs\.hlsl")
        compile_hlsl_profile(${ITEM} hs)
    elseif(${ITEM} MATCHES "\.lib\.hlsl")
        compile_hlsl_profile(${ITEM} lib)
    elseif(${ITEM} MATCHES "\.as\.hlsl")
        compile_hlsl_profile(${ITEM} as)
    elseif(${ITEM} MATCHES "\.ms\.hlsl")
        compile_hlsl_profile(${ITEM} ms)
    endif()
endforeach()

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
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.610.2/build/native
)
target_link_libraries(
    owge_d3d12_base PUBLIC
    d3d12.lib
    dxgi.lib
)

add_owge_lib(owge_asset)
target_include_directories(
    owge_d3d12_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.610.2/build/native
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
)

add_owge_exe(owge_tech_demo)
target_link_libraries(
    owge_tech_demo PUBLIC
    owge_render_engine
    owge_render_techniques
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
