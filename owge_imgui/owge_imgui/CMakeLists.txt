get_filename_component(IMGUI_DIR ../ ABSOLUTE)
get_filename_component(IMGUI_DIR ${IMGUI_DIR}/../ ABSOLUTE)
set(IMGUI_DIR ${IMGUI_DIR}/thirdparty/imgui)
target_sources(
    owge_imgui PRIVATE
    imgui_render_procedure.cpp
    imgui_render_procedure.hpp
    imgui_utils.cpp
    imgui_utils.hpp
    ${IMGUI_DIR}/imconfig.h
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_internal.h
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imstb_rectpack.h
    ${IMGUI_DIR}/imstb_textedit.h
    ${IMGUI_DIR}/imstb_truetype.h
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.h
    ${IMGUI_DIR}/backends/imgui_impl_dx12.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx12.h
)
