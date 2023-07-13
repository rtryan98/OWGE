#include "owge_render_techniques/ocean/ocean_render_resources.hpp"
#include "owge_render_techniques/ocean/ocean_simulation_render_procedure.hpp"
#include "owge_render_techniques/ocean/ocean_settings.hpp"

#include <owge_render_engine/render_engine.hpp>

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
}

void Ocean_Simulation_Render_Resources::destroy(Render_Engine* render_engine)
{
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
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .srv_dimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .uav_dimension = D3D12_UAV_DIMENSION_TEXTURE2D,
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
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .srv_dimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .uav_dimension = D3D12_UAV_DIMENSION_TEXTURE2D,
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
        .depth_or_array_layers = 1,
        .mip_levels = 1,
        .dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
        .srv_dimension = D3D12_SRV_DIMENSION_TEXTURE2D,
        .uav_dimension = D3D12_UAV_DIMENSION_TEXTURE2D,
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
