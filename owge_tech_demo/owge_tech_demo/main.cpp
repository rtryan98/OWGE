#include <cstdint>
#include <owge_d3d12_base/d3d12_util.hpp>
#include <owge_render_engine/render_engine.hpp>
#include <owge_window/window.hpp>

int32_t main()
{
    owge::Window window = {};
    std::unique_ptr<owge::Render_Engine> render_engine;
    owge::D3D12_Context_Settings d3d12_settings = {
        .enable_validation = true,
        .enable_gpu_based_validation = false,
        .disable_tdr = false,
        .d3d_feature_level = D3D_FEATURE_LEVEL_12_2,
        .static_samplers = {}
    };
    render_engine = std::make_unique<owge::Render_Engine>(window.get_hwnd(), d3d12_settings);
    return 0;
}
