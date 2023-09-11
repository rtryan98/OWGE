# add_custom_target(owge_shaders)
include(${CMAKE_CURRENT_LIST_DIR}/owge_general.cmake)

set(DXC_PATH "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/dxc_2023_03_01/bin/x64/dxc.exe") # Change path depending on OS.
set(SHADER_COMPILE_PARAMS -HV 2021 -Zpr -no-legacy-cbuf-layout -enable-16bit-types -I "${CMAKE_CURRENT_SOURCE_DIR}/owge_shaders/")
function(compile_hlsl SHADER MODEL ENTRYPOINT OUTFILE)
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
endfunction()

function(compile_hlsl_profile SHADER PROFILE)
    string(REPLACE ".hlsl" ".bin" COMPILE_SHADER_OUT ${SHADER})
    if(${SHADER} MATCHES "owge_shaders/owge_shaders")
        string(REPLACE "owge_shaders/owge_shaders" "res/builtin/shader" COMPILE_SHADER_OUT ${COMPILE_SHADER_OUT})
    endif()
    compile_hlsl(${SHADER} ${PROFILE}_6_6 "${PROFILE}_main" ${COMPILE_SHADER_OUT})
endfunction()

function(add_owge_shader_lib TARGET)
    add_custom_target(${TARGET})
    add_subdirectory(${TARGET})
    target_group_file_tree(${TARGET})
    get_target_property(${TARGET}_SOURCES owge_shaders SOURCES)
    foreach(ITEM IN ITEMS ${${TARGET}_SOURCES})
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
endfunction()
