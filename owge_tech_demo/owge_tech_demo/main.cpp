#include <chrono>
#include <cstdint>
#include <owge_d3d12_base/d3d12_util.hpp>
#include <owge_render_engine/render_engine.hpp>
#include <owge_render_engine/render_procedure/ocean.hpp>
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
        .enable_validation = false,
        .enable_gpu_based_validation = false,
        .disable_tdr = false,
        .d3d_feature_level = D3D_FEATURE_LEVEL_12_2,
        .static_samplers = {}
    };
    owge::Render_Engine_Settings render_engine_settings = {
        .nvperf_enabled = false,
        .nvperf_lock_clocks_to_rated_tdp = false
    };
    auto render_engine = std::make_unique<owge::Render_Engine>(
        window->get_hwnd(),
        d3d12_settings,
        render_engine_settings);

    owge::Swapchain_Pass_Settings swapchain_pass_settings = {
        .clear_color = { 0.25f, 0.25f, 0.75f, 0.0f }
    };
    auto swapchain_pass = std::make_unique<owge::Swapchain_Pass_Render_Procedure>(
        swapchain_pass_settings);
    owge::Ocean_Settings ocean_settings = {

    };
    owge::Ocean_Spectrum_Ocean_Parameters ocean_spectrum_ocean_parameters = {
        .size = 256,
        .length_scale = 128.0f,
        .gravity = 9.81f,
        .ocean_depth = 35.0f,
        .spectra = {
            {
                .wind_speed = 45.0f,
                .fetch = 512.0f
            },
            {
                .wind_speed = 45.0f,
                .fetch = 32768.0f
            }
        }
    };
    owge::Ocean_Tile_Settings ocean_tile_settings = {

    };
    auto ocean_calculate_spectra_render_procedure =
        std::make_unique<owge::Ocean_Calculate_Spectra_Render_Procedure>(
            render_engine.get(), ocean_spectrum_ocean_parameters);
    auto ocean_tile_render_procedure =
        std::make_unique<owge::Ocean_Tile_Render_Procedure>();

    render_engine->add_procedure(ocean_calculate_spectra_render_procedure.get());
    render_engine->add_procedure(swapchain_pass.get());
    swapchain_pass->add_subprocedure(ocean_tile_render_procedure.get());

    auto current_time = std::chrono::system_clock::now();
    auto last_time = current_time;
    while (window->get_data().alive)
    {
        window->poll_events();

        float delta_time = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_time).count();

        render_engine->render(delta_time);

        last_time = current_time;
        current_time = std::chrono::system_clock::now();
    }
    return 0;
}
