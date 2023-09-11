function(download_extract URL DST_FOLDER FOLDER_NAME)
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
endfunction()

function(copy_if_not_exist SRC DST)
    if(NOT EXISTS ${DST}) # Should check for contents in case of directory.
        message(STATUS "Copying ${SRC} to ${DST} .")
        file(COPY ${SRC} DESTINATION ${DST})
    else()
        message(STATUS "${SRC} already exists at ${DST} .")
    endif()
endfunction()

function(copy_dll_if_not_exist SRC DST)
    foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
        copy_if_not_exist(${SRC} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${OUTPUTCONFIG}/${DST})
    endforeach()
endfunction()

function(set_target_default_properties TARGET)
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
        target_compile_definitions(
            ${TARGET} PRIVATE
            NOMINMAX
            WIN32_LEAN_AND_MEAN
        )
    endif()
endfunction()

function(group_file_tree FILES DUPE_FILTER)
    foreach(ITEM IN ITEMS ${FILES})
        get_filename_component(SRC_PATH "${ITEM}" PATH)
        string(REPLACE "${CMAKE_SOURCE_DIR}" "" GROUP_PATH "${SRC_PATH}")
        string(REPLACE "${DUPE_FILTER}/${DUPE_FILTER}" "${DUPE_FILTER}" GROUP_PATH "${GROUP_PATH}")
        string(REPLACE "/" "\\" GROUP_PATH "${GROUP_PATH}")
        source_group("${GROUP_PATH}" FILES ${ITEM})
    endforeach()
endfunction()

function(target_group_file_tree TARGET)
    get_target_property(${TARGET}_SOURCES ${TARGET} SOURCES)
    group_file_tree("${${TARGET}_SOURCES}" "${TARGET}")
endfunction()

function(setup_owge_target TARGET)
    set_target_default_properties(${TARGET})
    add_subdirectory(${TARGET})
    target_group_file_tree(${TARGET})
    target_include_directories(
        ${TARGET} PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/${TARGET}
    )
endfunction()

function(add_owge_lib TARGET)
    add_library(${TARGET})
    setup_owge_target(${TARGET})
endfunction()

function(add_owge_exe TARGET)
    add_executable(${TARGET})
    setup_owge_target(${TARGET})
endfunction()
