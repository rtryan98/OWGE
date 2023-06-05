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
]]

set(NvPerfUtility_FOUND ON)
message(STATUS "Found NvPerfUtility")

SET(NvPerfUtility_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../NvPerfUtility/include")
SET(NvPerfUtility_IMPORTS_DIR "${CMAKE_CURRENT_LIST_DIR}/../NvPerfUtility/imports")

add_library(NvPerfUtility INTERFACE)
set_target_properties(NvPerfUtility PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${NvPerfUtility_INCLUDE_DIRS}")
SET(NvPerfUtility_LIBRARIES NvPerfUtility)


SET(NvPerfUtility_IMPORTS_IMGUI_DIR "${CMAKE_CURRENT_LIST_DIR}/../NvPerfUtility/imports/imgui-1.87")
set(NvPerfUtility_IMPORTS_IMGUI_SOURCES
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/imgui.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/imgui_draw.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/imgui_tables.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/imgui_widgets.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/backends/imgui_impl_dx12.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${NvPerfUtility_IMPORTS_IMGUI_DIR}/backends/imgui_impl_win32.cpp
)
add_library(NvPerfUtilityImportsImGui INTERFACE)
set_target_properties(
    NvPerfUtilityImportsImGui
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
            "${NvPerfUtility_IMPORTS_IMGUI_DIR};${NvPerfUtility_IMPORTS_IMGUI_DIR}/backends"
        INTERFACE_SOURCES
            "${NvPerfUtility_IMPORTS_IMGUI_SOURCES}"
)


SET(NvPerfUtility_IMPORTS_IMPLOT_DIR "${CMAKE_CURRENT_LIST_DIR}/../NvPerfUtility/imports/implot-0.13")
set(NvPerfUtility_IMPORTS_IMPLOT_SOURCES
    ${NvPerfUtility_IMPORTS_IMPLOT_DIR}/implot.cpp
    ${NvPerfUtility_IMPORTS_IMPLOT_DIR}/implot_items.cpp
)
add_library(NvPerfUtilityImportsImPlot INTERFACE)
set_target_properties(NvPerfUtilityImportsImPlot
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
            "${NvPerfUtility_IMPORTS_IMPLOT_DIR}"
        INTERFACE_SOURCES
            "${NvPerfUtility_IMPORTS_IMPLOT_SOURCES}"
)


SET(NvPerfUtility_IMPORTS_RYML_DIR "${CMAKE_CURRENT_LIST_DIR}/../NvPerfUtility/imports/rapidyaml-0.4.0")
add_library(NvPerfUtilityImportsRyml INTERFACE)
set_target_properties(NvPerfUtilityImportsRyml
    PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES
            "${NvPerfUtility_IMPORTS_RYML_DIR}"
)
