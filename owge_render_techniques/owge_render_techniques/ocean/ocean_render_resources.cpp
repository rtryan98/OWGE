#include "owge_render_techniques/ocean/ocean_render_resources.hpp"
#include "owge_render_techniques/ocean/ocean_simulation_render_procedure.hpp"
#include "owge_render_techniques/ocean/ocean_settings.hpp"

#include <owge_render_engine/render_engine.hpp>

#include <owge_asset/generator/plane_generator.hpp>

namespace owge
{
void Ocean_Simulation_Render_Resources::create(
    Render_Engine* render_engine,
    Ocean_Settings* settings)
{
    Shader_Desc fft_512_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\fft_512.cs.bin"
    };
    fft_512_shader = render_engine->create_shader(fft_512_shader_desc);
    Compute_Pipeline_Desc fft_512_pso_desc = {
        .cs = fft_512_shader
    };
    fft_512_pso = render_engine->create_pipeline(
        fft_512_pso_desc,
        L"PSO:Ocean:Compute:fft_512");

    Shader_Desc fft_256_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\fft_256.cs.bin"
    };
    fft_256_shader = render_engine->create_shader(fft_256_shader_desc);
    Compute_Pipeline_Desc fft_256_pso_desc = {
        .cs = fft_256_shader
    };
    fft_256_pso = render_engine->create_pipeline(
        fft_256_pso_desc,
        L"PSO:Ocean:Compute:fft_256");

    Shader_Desc fft_128_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\fft_128.cs.bin"
    };
    fft_128_shader = render_engine->create_shader(fft_128_shader_desc);
    Compute_Pipeline_Desc fft_128_pso_desc = {
        .cs = fft_128_shader
    };
    fft_128_pso = render_engine->create_pipeline(
        fft_128_pso_desc,
        L"PSO:Ocean:Compute:fft_128");

    Shader_Desc initial_spectrum_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\initial_spectrum.cs.bin"
    };
    initial_spectrum_shader = render_engine->create_shader(initial_spectrum_shader_desc);
    Compute_Pipeline_Desc initial_spectrum_pso_desc = {
        .cs = initial_spectrum_shader
    };
    initial_spectrum_pso = render_engine->create_pipeline(
        initial_spectrum_pso_desc,
        L"PSO:Ocean:Compute:Initial_Spectrum");

    Shader_Desc developed_spectrum_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\developed_spectrum.cs.bin"
    };
    developed_spectrum_shader = render_engine->create_shader(developed_spectrum_shader_desc);
    Compute_Pipeline_Desc developed_spectrum_pso_desc = {
        .cs = developed_spectrum_shader
    };
    developed_spectrum_pso = render_engine->create_pipeline(
        developed_spectrum_pso_desc,
        L"PSO:Ocean:Compute:Developed_Spectrum");

    Buffer_Desc initial_spectrum_params_buffer = {
        .size = sizeof(Ocean_Simulation_Initial_Spectrum_Parameter_Buffer),
        .heap_type = D3D12_HEAP_TYPE_DEFAULT,
        .usage = Resource_Usage::Read_Only
    };
    initial_spectrum_ocean_params_buffer = render_engine->create_buffer(
        initial_spectrum_params_buffer,
        L"Buffer:Ocean:Initial_Spectrum_Params");

    resize_textures(render_engine, settings);

    initial_spectrum_bindset = render_engine->create_bindset();
    developed_spectrum_bindset = render_engine->create_bindset();

    auto ocean_plane = mesh_generate_simple_plane_2d(512, 512);

    Buffer_Desc ocean_surface_vertex_buffer_desc = {
        .size = ocean_plane.vertex_positions.size() * sizeof(XMFLOAT2),
        .heap_type = D3D12_HEAP_TYPE_DEFAULT,
        .usage = Resource_Usage::Read_Only
    };
    ocean_surface_vertex_buffer = render_engine->create_buffer(ocean_surface_vertex_buffer_desc);
    render_engine->copy_and_upload_data(sizeof(XMFLOAT2) * ocean_plane.vertex_positions.size(),
        0, ocean_surface_vertex_buffer, 0, ocean_plane.vertex_positions.data());

    Buffer_Desc ocean_surface_index_buffer_desc = {
        .size = ocean_plane.indices.size() * sizeof(uint32_t),
        .heap_type = D3D12_HEAP_TYPE_DEFAULT,
        .usage = Resource_Usage::Read_Only
    };
    ocean_surface_index_buffer = render_engine->create_buffer(ocean_surface_index_buffer_desc);
    render_engine->copy_and_upload_data(sizeof(uint32_t) * ocean_plane.indices.size(),
        0, ocean_surface_index_buffer, 0, ocean_plane.indices.data());

    surface_render_vs_bindset = render_engine->create_bindset();
    Ocean_Surface_VS_Bindset surface_vs_bindset = {
        .vertex_buffer = uint32_t(ocean_surface_vertex_buffer.bindless_idx),
        .render_data = uint32_t(ocean_surface_vs_render_data.bindless_idx)
    };
    surface_render_vs_bindset.write_data(surface_vs_bindset);
    render_engine->update_bindings(surface_render_vs_bindset);

    surface_render_ps_bindset = render_engine->create_bindset();
    Ocean_Surface_PS_Bindset surface_ps_bindset = {
    };
    surface_render_ps_bindset.write_data(surface_ps_bindset);
    render_engine->update_bindings(surface_render_ps_bindset);

    Shader_Desc surface_plane_vs_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\surface_render.vs.bin"
    };
    surface_plane_vs = render_engine->create_shader(surface_plane_vs_desc);
    Shader_Desc surface_plane_ps_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\surface_render.ps.bin"
    };
    surface_plane_ps = render_engine->create_shader(surface_plane_ps_desc);
    Graphics_Pipeline_Desc surface_plane_pso_desc = {
        .shaders = {
            .vs = surface_plane_vs,
            .ps = surface_plane_ps
        },
        .primitive_topology_type = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
        .blend_state = {
            .AlphaToCoverageEnable = false,
            .IndependentBlendEnable = false,
            .RenderTarget = {
                {
                    .BlendEnable = false,
                    .LogicOpEnable = false,
                    .RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
                }
            }
        },
        .rasterizer_state = {
            .FillMode = D3D12_FILL_MODE_SOLID,
            .CullMode = D3D12_CULL_MODE_BACK,
            .FrontCounterClockwise = true
        },
        .depth_stencil_state = {
            .DepthEnable = false,
            .StencilEnable = false
        },
        .rtv_count = 1,
        .rtv_formats = {
            DXGI_FORMAT_R8G8B8A8_UNORM
        },
        .dsv_format = DXGI_FORMAT_UNKNOWN
    };
    surface_plane_pso = render_engine->create_pipeline(surface_plane_pso_desc);
}

void Ocean_Simulation_Render_Resources::destroy(Render_Engine* render_engine)
{
    render_engine->destroy_pipeline(surface_plane_pso);
    render_engine->destroy_shader(surface_plane_ps);
    render_engine->destroy_shader(surface_plane_vs);

    render_engine->destroy_bindset(surface_render_ps_bindset);
    render_engine->destroy_bindset(surface_render_vs_bindset);

    render_engine->destroy_buffer(ocean_surface_index_buffer);
    render_engine->destroy_buffer(ocean_surface_vertex_buffer);

    render_engine->destroy_bindset(developed_spectrum_bindset);
    render_engine->destroy_bindset(initial_spectrum_bindset);

    render_engine->destroy_buffer(initial_spectrum_ocean_params_buffer);

    render_engine->destroy_texture(developed_spectrum_texture);
    render_engine->destroy_texture(angular_frequency_texture);
    render_engine->destroy_texture(initial_spectrum_texture);

    render_engine->destroy_pipeline(developed_spectrum_pso);
    render_engine->destroy_pipeline(initial_spectrum_pso);
    render_engine->destroy_pipeline(fft_512_pso);
    render_engine->destroy_pipeline(fft_256_pso);
    render_engine->destroy_pipeline(fft_128_pso);

    render_engine->destroy_shader(developed_spectrum_shader);
    render_engine->destroy_shader(initial_spectrum_shader);
    render_engine->destroy_shader(fft_512_shader);
    render_engine->destroy_shader(fft_256_shader);
    render_engine->destroy_shader(fft_128_shader);
}

void Ocean_Simulation_Render_Resources::resize_textures(Render_Engine* render_engine, Ocean_Settings* settings)
{
    if (!initial_spectrum_texture.is_null_handle())
    {
        render_engine->destroy_texture(initial_spectrum_texture);
        initial_spectrum_texture = {};
    }
    Texture_Desc initial_spectrum_texture_desc = {
        .width = settings->size,
        .height = settings->size,
        .depth_or_array_layers = Ocean_Settings::MAX_CASCADES,
        .mip_levels = 1,
        .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .srv_dimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
        .uav_dimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
        .rtv_dimension = D3D12_RTV_DIMENSION_UNKNOWN,
        .dsv_dimension = D3D12_DSV_DIMENSION_UNKNOWN,
        .initial_layout = D3D12_BARRIER_LAYOUT_UNDEFINED,
        .format = DXGI_FORMAT_R32G32B32A32_FLOAT
    };
    initial_spectrum_texture = render_engine->create_texture(
        initial_spectrum_texture_desc,
        L"Texture:Ocean:Initial_Spectrum");

    if (!angular_frequency_texture.is_null_handle())
    {
        render_engine->destroy_texture(angular_frequency_texture);
        angular_frequency_texture = {};
    }
    Texture_Desc angular_frequency_texture_desc = {
        .width = settings->size,
        .height = settings->size,
        .depth_or_array_layers = Ocean_Settings::MAX_CASCADES,
        .mip_levels = 1,
        .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .srv_dimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
        .uav_dimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
        .rtv_dimension = D3D12_RTV_DIMENSION_UNKNOWN,
        .dsv_dimension = D3D12_DSV_DIMENSION_UNKNOWN,
        .initial_layout = D3D12_BARRIER_LAYOUT_UNDEFINED,
        .format = DXGI_FORMAT_R32_FLOAT
    };
    angular_frequency_texture = render_engine->create_texture(
        angular_frequency_texture_desc,
        L"Texture:Ocean:Angular_Frequency");

    if (!developed_spectrum_texture.is_null_handle())
    {
        render_engine->destroy_texture(developed_spectrum_texture);
        developed_spectrum_texture = {};
    }
    Texture_Desc developed_spectrum_texture_desc = {
        .width = settings->size,
        .height = settings->size,
        .depth_or_array_layers = Ocean_Settings::MAX_CASCADES,
        .mip_levels = 1,
        .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .srv_dimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY,
        .uav_dimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY,
        .rtv_dimension = D3D12_RTV_DIMENSION_UNKNOWN,
        .dsv_dimension = D3D12_DSV_DIMENSION_UNKNOWN,
        .initial_layout = D3D12_BARRIER_LAYOUT_UNDEFINED,
        .format = DXGI_FORMAT_R32G32_FLOAT
    };
    developed_spectrum_texture = render_engine->create_texture(
        developed_spectrum_texture_desc,
        L"Texture:Ocean:Developed_Spectrum");
}
}
