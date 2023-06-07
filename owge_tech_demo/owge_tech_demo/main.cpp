#include <cstdint>
#include <owge_d3d12_base/d3d12_util.hpp>
#include <owge_render_engine/render_engine.hpp>
#include <owge_window/window.hpp>

int32_t main()
{
    owge::Window_Settings window_settings = {
        .width = 1920,
        .height = 1080,
        .title = "OWGE Tech Demo",
        .userproc = nullptr
    };
    auto window = std::make_unique<owge::Window>(
        window_settings);
    owge::D3D12_Context_Settings d3d12_settings = {
        .enable_validation = true,
        .enable_gpu_based_validation = false,
        .disable_tdr = false,
        .d3d_feature_level = D3D_FEATURE_LEVEL_12_2,
        .static_samplers = {}
    };
    owge::Render_Engine_Settings render_engine_settings = {
        .nvperf_enabled = true,
        .nvperf_lock_clocks_to_rated_tdp = false
    };
    auto render_engine = std::make_unique<owge::Render_Engine>(
        window->get_hwnd(),
        d3d12_settings,
        render_engine_settings);
    while (window->get_data().alive)
    {
        window->poll_events();
    }
    return 0;
}
