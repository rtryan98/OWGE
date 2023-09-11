#include <chrono>
#include <cstdint>
#include <owge_render_engine/camera.hpp>
#include <owge_d3d12_base/d3d12_util.hpp>
#include <owge_render_engine/render_engine.hpp>
#include <owge_render_engine/render_procedure/swapchain_pass.hpp>
#include <owge_window/window.hpp>
#include <owge_imgui/imgui_utils.hpp>
#include <owge_imgui/imgui_render_procedure.hpp>
#include <owge_render_techniques/render_technique_settings.hpp>

#include <owge_window/input.hpp>

#include <owge_render_techniques/ocean/ocean_settings.hpp>
#include <owge_render_techniques/ocean/ocean_simulation_render_procedure.hpp>
#include <owge_render_techniques/ocean/ocean_render_resources.hpp>
#include <owge_render_techniques/ocean/ocean_surface_render_procedure.hpp>

#include <tclap/CmdLine.h>
#include <tclap/SwitchArg.h>
#include <tclap/UnlabeledValueArg.h>

#undef near // Really windows?
#undef far

int32_t main(uint32_t argc, const char* argv[])
{
    SetProcessDPIAware();

    TCLAP::CmdLine cmd_line("OWGE Tech Demo");
    // cmd_line.setOutput(nullptr); Set nullptr for deployment.
    TCLAP::SwitchArg enable_d3d12_validation_arg(
        "",
        "d3d12_validation_enable",
        "Enable validation layers for D3D12.",
        cmd_line, false);
    TCLAP::SwitchArg enable_d3d12_gpu_validation_arg(
        "",
        "d3d12_gpu_based_validation_enable",
        "Enable GPU based validation for D3D12. Implies 'd3d12_validation_enable'.",
        cmd_line, false);
    TCLAP::SwitchArg enable_nvperf_arg(
        "",
        "nvperf_enable",
        "Enable profiling with NVIDIA Nsight Perf. "
        "Incompatible with 'd3d12_validation_enable' and 'd3d12_gpu_based_validation_enable'.",
        cmd_line, false);
    TCLAP::SwitchArg lock_clocks_to_tdp_arg(
        "",
        "nvperf_lock_clocks_to_tdp",
        "Lock GPU clocks to TDP. Implies 'nvperf_enable'.",
        cmd_line, false);
    cmd_line.parse(argc, argv);

    owge::Window_Settings window_settings = {
        .width = 1920,
        .height = 1080,
        .title = "OWGE Tech Demo",
        .userproc = ImGui_ImplWin32_WndProcHandler
    };
    auto window = std::make_unique<owge::Window>(
        window_settings);
    auto input = std::make_unique<owge::Input>(window->get_hwnd());
    owge::D3D12_Context_Settings d3d12_settings = {
        .enable_validation = enable_d3d12_gpu_validation_arg.getValue() || enable_d3d12_validation_arg.getValue(),
        .enable_gpu_based_validation = enable_d3d12_gpu_validation_arg.getValue(),
        .disable_tdr = false,
        .d3d_feature_level = D3D_FEATURE_LEVEL_12_1,
        .static_samplers = {}
    };
    owge::Render_Engine_Settings render_engine_settings = {
        .nvperf_enabled = d3d12_settings.enable_validation ? false : enable_nvperf_arg.getValue(),
        .nvperf_lock_clocks_to_rated_tdp = false
    };
    auto render_engine = std::make_unique<owge::Render_Engine>(
        window->get_hwnd(),
        d3d12_settings,
        render_engine_settings);

    auto window_data = window->get_data();
    owge::Texture_Desc ds_tex_desc = {
        .width = window_data.width,
        .height = window_data.height,
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .srv_dimension = D3D12_SRV_DIMENSION_UNKNOWN,
        .uav_dimension = D3D12_UAV_DIMENSION_UNKNOWN,
        .rtv_dimension = D3D12_RTV_DIMENSION_UNKNOWN,
        .dsv_dimension = D3D12_DSV_DIMENSION_TEXTURE2D,
        .initial_layout = D3D12_BARRIER_LAYOUT_UNDEFINED,
        .format = DXGI_FORMAT_D32_FLOAT,
        .optimized_clear_value = {
            .Format = DXGI_FORMAT_D32_FLOAT,
            .DepthStencil = {
                .Depth = 1.0f,
                .Stencil = 0
            }
        }
    };
    auto ds_texture = render_engine->create_texture(ds_tex_desc);

    owge::Swapchain_Pass_Settings swapchain_pass_settings = {
        .clear_color = { 0.02315f, 0.30383f, 0.5911f, 0.0f },
        .depth_stencil_texture = ds_texture
    };
    auto swapchain_pass = std::make_unique<owge::Swapchain_Pass_Render_Procedure>(
        swapchain_pass_settings);

    auto camera = owge::Simple_Fly_Camera{
        .fov_y = 90.0f,
        .near = 0.01f,
        .far = 1000.0f,
        .sensitivity = 0.25f,
        .movement_speed = 10.0f
    };

    auto ocean_resources = owge::Ocean_Simulation_Render_Resources{};
    auto ocean_render_technique_settings = owge::Ocean_Render_Technique_Settings(render_engine.get(), &ocean_resources);
    ocean_resources.create(render_engine.get(), &ocean_render_technique_settings.settings);
    auto ocean_simulation_render_procedure = std::make_unique<owge::Ocean_Simulation_Render_Procedure>(
        &ocean_render_technique_settings.settings, &ocean_resources
        );
    auto ocean_surface_render_procedure = std::make_unique<owge::Ocean_Surface_Render_Procedure>(
        &ocean_render_technique_settings.settings, &ocean_resources, &camera.camera_data
        );

    auto imgui_render_procedure =
        std::make_unique<owge::Imgui_Render_Procedure>();

    render_engine->add_procedure(ocean_simulation_render_procedure.get());
    render_engine->add_procedure(swapchain_pass.get());
    swapchain_pass->add_subprocedure(ocean_surface_render_procedure.get());
    swapchain_pass->add_subprocedure(imgui_render_procedure.get());

    owge::imgui_init(window->get_hwnd(), render_engine.get());

    auto current_time = std::chrono::system_clock::now();
    auto last_time = current_time;
    while (window->get_data().alive)
    {
        window->poll_events();
        input->update_input_state();
        owge::imgui_new_frame();
        bool imgui_capture_input = ImGui::GetIO().WantCaptureMouse;

        float delta_time = std::chrono::duration_cast<std::chrono::duration<float>>(current_time - last_time).count();
        camera.aspect = float(window->get_data().width) / float(window->get_data().height);
        camera.update(input.get(), delta_time, !imgui_capture_input);

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

    render_engine->destroy_texture(ds_texture);
    ocean_resources.destroy(render_engine.get());

    return 0;
}
