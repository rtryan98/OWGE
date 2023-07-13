#include <chrono>
#include <cstdint>
#include <owge_d3d12_base/d3d12_util.hpp>
#include <owge_render_engine/render_engine.hpp>
#include <owge_render_engine/render_procedure/swapchain_pass.hpp>
#include <owge_window/window.hpp>
#include <owge_imgui/imgui_utils.hpp>
#include <owge_imgui/imgui_render_procedure.hpp>
#include <owge_render_techniques/render_technique_settings.hpp>

#include <owge_render_techniques/ocean/ocean_settings.hpp>
#include <owge_render_techniques/ocean/ocean_simulation_render_procedure.hpp>
#include <owge_render_techniques/ocean/ocean_render_resources.hpp>

int32_t main()
{
    owge::Window_Settings window_settings = {
        .width = 1920,
        .height = 1080,
        .title = "OWGE Tech Demo",
        .userproc = ImGui_ImplWin32_WndProcHandler
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

    auto ocean_resources = owge::Ocean_Simulation_Render_Resources{};
    auto ocean_render_technique_settings = owge::Ocean_Render_Technique_Settings(render_engine.get(), &ocean_resources);
    ocean_resources.create(render_engine.get(), &ocean_render_technique_settings.settings);
    auto ocean_simulation_render_procedure = std::make_unique<owge::Ocean_Simulation_Render_Procedure>(
        &ocean_render_technique_settings.settings, &ocean_resources
        );

    auto imgui_render_procedure =
        std::make_unique<owge::Imgui_Render_Procedure>();

    render_engine->add_procedure(ocean_simulation_render_procedure.get());
    render_engine->add_procedure(swapchain_pass.get());
    swapchain_pass->add_subprocedure(imgui_render_procedure.get());

    owge::imgui_init(window->get_hwnd(), render_engine.get());

    auto current_time = std::chrono::system_clock::now();
    auto last_time = current_time;
    while (window->get_data().alive)
    {
        window->poll_events();
        owge::imgui_new_frame();

        float delta_time = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_time).count();

        ImGui::ShowDemoWindow();

        static bool renderer_settings_open = true;
        ImGui::SetNextWindowSizeConstraints(ImVec2(512.0f, 512.0f), ImVec2(2.0f * 512.0f, 2.0f * 512.0f));
        ImGui::Begin("Renderer Settings", &renderer_settings_open,
            ImGuiWindowFlags_HorizontalScrollbar);
        ocean_render_technique_settings.on_gui();
        ImGui::End();

        render_engine->render(delta_time);

        last_time = current_time;
        current_time = std::chrono::system_clock::now();
    }

    owge::imgui_shutdown();
    ocean_resources.destroy(render_engine.get());

    return 0;
}
