#include <cstdint>
#include <owge_d3d12_base/d3d12_util.hpp>
#include <owge_render_engine/render_engine.hpp>
#include <owge_render_engine/render_procedure/swapchain_pass.hpp>
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
        .d3d_feature_level = D3D_FEATURE_LEVEL_12_1,
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
    owge::Swapchain_Pass_Settings swapchain_pass_settings = {
        .clear_color = { 0.1250f, 0.0f, 0.0f, 0.0f }
    };
    auto swapchain_pass = std::make_unique<owge::Swapchain_Pass>(
        swapchain_pass_settings);

    render_engine->add_procedure(swapchain_pass.get());

    while (window->get_data().alive)
    {
        window->poll_events();
        render_engine->render();
    }
    return 0;
}
