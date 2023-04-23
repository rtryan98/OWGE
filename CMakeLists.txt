cmake_minimum_required(VERSION 3.18)
project(OWGE)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY bin/)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY bin/)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY bin/)

macro(download_extract URL DST_FOLDER FOLDER_NAME)
    if(NOT EXISTS ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip)
        message(STATUS "Downloading ${URL} and unpacking to ${DST_FOLDER}/${FOLDER_NAME}.")
        file(
            DOWNLOAD
            ${URL}
            ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip
        )
    else()
        message(STATUS "${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip already exists. No download required.")
    endif()
    if(NOT EXISTS ${DST_FOLDER}/${FOLDER_NAME})
        message(STATUS "Extracting ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip to ${DST_FOLDER}/${FOLDER_NAME}.")
        file(
            ARCHIVE_EXTRACT
            INPUT ${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip
            DESTINATION ${DST_FOLDER}/${FOLDER_NAME}
        )
    else()
        message(STATUS "${CMAKE_BINARY_DIR}/download/${FOLDER_NAME}.zip is already extracted to ${DST_FOLDER}/${FOLDER_NAME}.")
    endif()
endmacro()

macro(copy_if_not_exist SRC DST)
    if(NOT EXISTS ${DST}) # Should check for contents in case of directory.
        message(STATUS "Copying ${SRC} to ${DST}.")
        file(COPY ${SRC} DESTINATION ${DST})
    else()
        message(STATUS "${SRC} already exists at ${DST}.")
    endif()
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

copy_if_not_exist(
    ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/d3d12_agility_1.610.2/build/native/bin/x64/
    ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/D3D12
)
