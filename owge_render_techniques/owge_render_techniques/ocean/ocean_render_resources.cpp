#include "owge_render_techniques/ocean/ocean_render_resources.hpp"
#include "owge_render_techniques/ocean/ocean_simulation_render_procedure.hpp"

#include <owge_render_engine/render_engine.hpp>

#include <owge_asset/generator/plane_generator.hpp>

namespace owge
{
void Ocean_Simulation_Render_Resources::create(
    Render_Engine* render_engine,
    Ocean_Settings* settings)
{
    create_simulation_shaders(render_engine);
    create_simulation_resources(render_engine);
    create_surface_shaders(render_engine);
    create_surface_resources(render_engine);

    resize_textures(render_engine, settings);
}

void Ocean_Simulation_Render_Resources::destroy(Render_Engine* render_engine)
{
    destroy_surface_resources(render_engine);
    destroy_surface_shaders(render_engine);
    destroy_simulation_resources(render_engine);
    destroy_simulation_shaders(render_engine);
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

    if (!displacement_x_y_z_texture.is_null_handle())
    {
        render_engine->destroy_texture(displacement_x_y_z_texture);
        displacement_x_y_z_texture = {};
    }
    Texture_Desc displacement_x_y_z_texture_desc = {
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
    displacement_x_y_z_texture = render_engine->create_texture(
        displacement_x_y_z_texture_desc,
        L"Texture:Ocean:Displacement_X_Y_Z");

    if (!derivatives_texture.is_null_handle())
    {
        render_engine->destroy_texture(derivatives_texture);
        derivatives_texture = {};
    }
    Texture_Desc derivatives_texture_desc = {
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
    derivatives_texture = render_engine->create_texture(
        displacement_x_y_z_texture_desc,
        L"Texture:Ocean:Derivatives");

    if (!jacobian_texture.is_null_handle())
    {
        render_engine->destroy_texture(jacobian_texture);
        jacobian_texture = {};
    }
    Texture_Desc jacobian_texture_desc = {
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
    jacobian_texture = render_engine->create_texture(
        displacement_x_y_z_texture_desc,
        L"Texture:Ocean:Jacobian");

    if (!packed_x_y_texture.is_null_handle())
    {
        render_engine->destroy_texture(packed_x_y_texture);
        packed_x_y_texture = {};
    }
    if (!packed_z_x_dx_texture.is_null_handle())
    {
        render_engine->destroy_texture(packed_z_x_dx_texture);
        packed_z_x_dx_texture = {};
    }
    if (!packed_y_dx_z_dx_texture.is_null_handle())
    {
        render_engine->destroy_texture(packed_y_dx_z_dx_texture);
        packed_y_dx_z_dx_texture = {};
    }
    if (!packed_y_dy_z_dy_texture.is_null_handle())
    {
        render_engine->destroy_texture(packed_y_dy_z_dy_texture);
        packed_y_dy_z_dy_texture = {};
    }
    Texture_Desc packed_texture_desc = {
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
    packed_x_y_texture = render_engine->create_texture(
        packed_texture_desc,
        L"Texture:Ocean:Packed_x_y");
    packed_z_x_dx_texture = render_engine->create_texture(
        packed_texture_desc,
        L"Texture:Ocean:Packed_z_x_dx");
    packed_y_dx_z_dx_texture = render_engine->create_texture(
        packed_texture_desc,
        L"Texture:Ocean:Packed_y_dx_z_dx");
    packed_y_dy_z_dy_texture = render_engine->create_texture(
        packed_texture_desc,
        L"Texture:Ocean:Packed_y_dy_z_dy");

    update_persistent_bindsets(render_engine);
}

void Ocean_Simulation_Render_Resources::create_simulation_shaders(Render_Engine* render_engine)
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

    Shader_Desc texture_reorder_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\texture_reorder.cs.bin"
    };
    texture_reorder_shader = render_engine->create_shader(texture_reorder_shader_desc);
    Compute_Pipeline_Desc texture_reorder_pso_desc = {
        .cs = texture_reorder_shader
    };
    texture_reorder_pso = render_engine->create_pipeline(texture_reorder_pso_desc,
        L"PSO:Ocean:Compute:Texture_Reorder");
}

void Ocean_Simulation_Render_Resources::destroy_simulation_shaders(Render_Engine* render_engine)
{
    render_engine->destroy_pipeline(texture_reorder_pso);
    render_engine->destroy_pipeline(developed_spectrum_pso);
    render_engine->destroy_pipeline(initial_spectrum_pso);
    render_engine->destroy_pipeline(fft_512_pso);
    render_engine->destroy_pipeline(fft_256_pso);
    render_engine->destroy_pipeline(fft_128_pso);

    render_engine->destroy_shader(texture_reorder_shader);
    render_engine->destroy_shader(developed_spectrum_shader);
    render_engine->destroy_shader(initial_spectrum_shader);
    render_engine->destroy_shader(fft_512_shader);
    render_engine->destroy_shader(fft_256_shader);
    render_engine->destroy_shader(fft_128_shader);
}

void Ocean_Simulation_Render_Resources::create_simulation_resources(Render_Engine* render_engine)
{
    Buffer_Desc initial_spectrum_params_buffer = {
        .size = sizeof(Ocean_Simulation_Initial_Spectrum_Parameter_Buffer),
        .heap_type = D3D12_HEAP_TYPE_DEFAULT,
        .usage = Resource_Usage::Read_Only
    };
    initial_spectrum_ocean_params_buffer = render_engine->create_buffer(
        initial_spectrum_params_buffer,
        L"Buffer:Ocean:Initial_Spectrum_Params");

    initial_spectrum_bindset = render_engine->create_bindset();
    developed_spectrum_bindset = render_engine->create_bindset();
    texture_reorder_bindset = render_engine->create_bindset();
}

void Ocean_Simulation_Render_Resources::destroy_simulation_resources(Render_Engine* render_engine)
{
    render_engine->destroy_bindset(texture_reorder_bindset);
    render_engine->destroy_bindset(developed_spectrum_bindset);
    render_engine->destroy_bindset(initial_spectrum_bindset);
    render_engine->destroy_buffer(initial_spectrum_ocean_params_buffer);

    render_engine->destroy_texture(packed_x_y_texture);
    render_engine->destroy_texture(packed_z_x_dx_texture);
    render_engine->destroy_texture(packed_y_dx_z_dx_texture);
    render_engine->destroy_texture(packed_y_dy_z_dy_texture);

    render_engine->destroy_texture(angular_frequency_texture);
    render_engine->destroy_texture(initial_spectrum_texture);
}

void Ocean_Simulation_Render_Resources::create_surface_shaders(Render_Engine* render_engine)
{
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
            .DepthEnable = true,
            .DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
            .DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
            .StencilEnable = false,
        },
        .rtv_count = 1,
        .rtv_formats = {
            DXGI_FORMAT_R8G8B8A8_UNORM
        },
        .dsv_format = DXGI_FORMAT_D32_FLOAT
    };
    surface_plane_pso = render_engine->create_pipeline(surface_plane_pso_desc);
}

void Ocean_Simulation_Render_Resources::destroy_surface_shaders(Render_Engine* render_engine)
{
    render_engine->destroy_pipeline(surface_plane_pso);
    render_engine->destroy_shader(surface_plane_ps);
    render_engine->destroy_shader(surface_plane_vs);
}

void Ocean_Simulation_Render_Resources::create_surface_resources(Render_Engine* render_engine)
{
    auto ocean_plane = mesh_generate_simple_plane_2d(4096);

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

    ocean_surface_index_count = uint32_t(ocean_plane.indices.size());

    Buffer_Desc ocean_surface_vs_render_data_buffer_desc = {
        .size = sizeof(Ocean_Surface_VS_Render_Data),
        .heap_type = D3D12_HEAP_TYPE_DEFAULT,
        .usage = Resource_Usage::Read_Only
    };
    ocean_surface_vs_render_data_buffer = render_engine->create_buffer(ocean_surface_vs_render_data_buffer_desc);

    Sampler_Desc ocean_surface_sampler_desc = {
        .filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,
        .address_u = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .address_v = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .address_w = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
        .mip_lod_bias = 0.0f,
        .max_anisotropy = 0,
        .comparison_func = D3D12_COMPARISON_FUNC_NONE,
        .boder_color = {},
        .min_lod = 0.0f,
        .max_lod = 0.0f
    };
    ocean_surface_sampler = render_engine->create_sampler(ocean_surface_sampler_desc);

    surface_render_vs_bindset = render_engine->create_bindset();
    surface_render_ps_bindset = render_engine->create_bindset();
}

void Ocean_Simulation_Render_Resources::destroy_surface_resources(Render_Engine* render_engine)
{
    render_engine->destroy_bindset(surface_render_ps_bindset);
    render_engine->destroy_bindset(surface_render_vs_bindset);
    render_engine->destroy_sampler(ocean_surface_sampler);
    render_engine->destroy_buffer(ocean_surface_vs_render_data_buffer);
    render_engine->destroy_buffer(ocean_surface_index_buffer);
    render_engine->destroy_buffer(ocean_surface_vertex_buffer);

    render_engine->destroy_texture(displacement_x_y_z_texture);
    render_engine->destroy_texture(derivatives_texture);
    render_engine->destroy_texture(jacobian_texture);
}

void Ocean_Simulation_Render_Resources::update_persistent_bindsets(Render_Engine* render_engine)
{
    Ocean_Texture_Reorder_Shader_Bindset texture_reorder_bindset_data = {
        .packed_x_y       = uint32_t(packed_x_y_texture.bindless_idx),
        .packed_z_x_dx    = uint32_t(packed_z_x_dx_texture.bindless_idx),
        .packed_y_dx_z_dx = uint32_t(packed_y_dx_z_dx_texture.bindless_idx),
        .packed_y_dy_z_dy = uint32_t(packed_y_dy_z_dy_texture.bindless_idx),
        .displacement     = uint32_t(displacement_x_y_z_texture.bindless_idx),
        .derivatives      = uint32_t(derivatives_texture.bindless_idx),
        .folding_map      = uint32_t(jacobian_texture.bindless_idx)
    };
    texture_reorder_bindset.write_data(texture_reorder_bindset_data);
    render_engine->update_bindings(texture_reorder_bindset);

    Ocean_Surface_Bindset surface_vs_bindset = {
        .vertex_buffer = uint32_t(ocean_surface_vertex_buffer.bindless_idx),
        .render_data = uint32_t(ocean_surface_vs_render_data_buffer.bindless_idx),
        .displacement_texture = uint32_t(displacement_x_y_z_texture.bindless_idx),
        .derivatives_texture = uint32_t(derivatives_texture.bindless_idx),
        .jacobian_texture = uint32_t(jacobian_texture.bindless_idx),
        .surface_sampler = uint32_t(ocean_surface_sampler.bindless_idx)
    };
    surface_render_vs_bindset.write_data(surface_vs_bindset);
    render_engine->update_bindings(surface_render_vs_bindset);
}

}
