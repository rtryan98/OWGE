cmake_minimum_required(VERSION 3.18)
project(OWGE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo;Release" CACHE STRING "" FORCE) # remove MinSizeRel

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
            VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
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

add_custom_target(owge_shaders)
set(DXC_PATH "thirdparty/dxc_2023_03_01/bin/x64/dxc.exe") # Change path depending on OS.
macro(compile_hlsl SHADER MODEL ENTRYPOINT OUTFILE)
    target_sources(owge_shaders PRIVATE ${owge_shader_sources} ${SHADER})
    if(MSVC)
        set_source_files_properties(${SHADER} PROPERTIES VS_TOOL_OVERRIDE "None")
    endif()
    add_custom_command(
        OUTPUT ${OUTFILE}
        COMMAND ${DXC_PATH} -T ${MODEL} -E ${ENTRYPOINT} -HV 2021 -ftime-report -I "owge_shaders/" -force-rootsig-ver rootsig_1_2 -Zpr -Fo ${OUTFILE} ${SHADER}
        MAIN_DEPENDENCY ${SHADER}
        COMMENT "Compiling ${SHADER} with parameters model ${MODEL}."
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        VERBATIM
    )
endmacro()

macro(compile_hlsl_profile SHADER PROFILE)
    string(REPLACE ".hlsl" ".${PROFILE}.bin" COMPILE_SHADER_OUT ${SHADER})
    compile_hlsl(${SHADER} ${PROFILE}_6_7 "${PROFILE}_main" ${COMPILE_SHADER_OUT})
endmacro()

download_extract(
    https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2212.1/dxc_2023_03_01.zip
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    dxc_2023_03_01
)

download_extract(
    https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.610.2
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty
    d3d12_agility_1.610.2
)

copy_dll_if_not_exist(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.610.2/build/native/bin/x64/
    D3D12
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

add_owge_lib(owge_d3d12_base)
target_include_directories(
    owge_d3d12_base PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.610.2/build/native/include
)
target_link_libraries(
    owge_d3d12_base PUBLIC
    d3d12.lib
    dxgi.lib
)

add_owge_lib(owge_window)

add_owge_lib(owge_render_engine)
target_include_directories(
    owge_render_engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/direct_storage_1.2.0/native/include
)
target_link_libraries(
    owge_render_engine PUBLIC
    owge_d3d12_base
    owge_window
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/direct_storage_1.2.0/native/lib/x64/dstorage.lib
)

add_owge_exe(owge_tech_demo)
target_link_libraries(
    owge_tech_demo PUBLIC
    owge_render_engine
)
add_dependencies(owge_tech_demo owge_shaders)
