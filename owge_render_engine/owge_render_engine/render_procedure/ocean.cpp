#include "owge_render_engine/render_procedure/ocean.hpp"
#include "owge_render_engine/render_engine.hpp"
#include "owge_render_engine/command_list.hpp"

namespace owge
{
float ocean_spectrum_calculate_v_yu_karaev_spectrum_omega_m(float fetch)
{
    double f = fetch;
    double result = 0.61826 + 0.0000003529 * f - 0.00197508
        * sqrt(f) + (62.554 / sqrt(f)) - (290.2 / f);
    return float(result);
}

struct Ocean_Initial_Spectrum_Shader_Bindset
{
    uint32_t ocean_params_buf_idx;
    uint32_t initial_spectrum_tex_idx;
    uint32_t angular_frequency_tex_idx;
};

struct Ocean_Developed_Spectrum_Shader_Bindset
{
    uint32_t initial_spectrum_tex_idx;
    uint32_t angular_frequency_tex_idx;
    uint32_t developed_spectrum_tex_idx;
    float time;
    uint32_t size;
};

Ocean_Calculate_Spectra_Render_Procedure::Ocean_Calculate_Spectra_Render_Procedure(
    Render_Engine* render_engine,
    const Ocean_Spectrum_Ocean_Parameters& ocean_spectrum_pars)
    : Render_Procedure("Ocean_Initial_Spectrum"),
    m_render_engine(render_engine),
    m_ocean_spectrum_pars(ocean_spectrum_pars)
{
    Shader_Desc fft_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\fft_512.cs.bin"
    };
    m_fft_shader = m_render_engine->create_shader(fft_shader_desc);
    Compute_Pipeline_Desc fft_compute_pso_desc = {
        .cs = m_fft_shader
    };
    m_fft_compute_pso = m_render_engine->create_pipeline(
        fft_compute_pso_desc,
        L"PSO:Ocean:Compute:fft_512");

    Shader_Desc initial_spectrum_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\initial_spectrum.cs.bin"
    };
    m_initial_spectrum_shader = m_render_engine->create_shader(initial_spectrum_shader_desc);
    Compute_Pipeline_Desc initial_spectrum_compute_pso_desc = {
        .cs = m_initial_spectrum_shader
    };
    m_initial_spectrum_compute_pso = m_render_engine->create_pipeline(
        initial_spectrum_compute_pso_desc,
        L"PSO:Ocean:Compute:Initial_Spectrum");

    Shader_Desc developed_spectrum_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\developed_spectrum.cs.bin"
    };
    m_developed_spectrum_shader = m_render_engine->create_shader(developed_spectrum_shader_desc);
    Compute_Pipeline_Desc developed_spectrum_compute_pso_desc = {
        .cs = m_developed_spectrum_shader
    };
    m_developed_spectrum_compute_pso = m_render_engine->create_pipeline(
        developed_spectrum_compute_pso_desc,
        L"PSO:Ocean:Compute:Developed_Spectrum");

    Texture_Desc initial_spectrum_texture_desc = {
        .width = m_ocean_spectrum_pars.size,
        .height = m_ocean_spectrum_pars.size,
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
    m_initial_spectrum_texture = m_render_engine->create_texture(
        initial_spectrum_texture_desc,
        L"Texture:Ocean:Initial_Spectrum");

    Texture_Desc angular_frequency_texture_desc = {
        .width = m_ocean_spectrum_pars.size,
        .height = m_ocean_spectrum_pars.size,
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
    m_angular_frequency_texture = m_render_engine->create_texture(
        angular_frequency_texture_desc,
        L"Texture:Ocean:Angular_Frequency");

    Texture_Desc developed_spectrum_texture_desc = {
        .width = m_ocean_spectrum_pars.size,
        .height = m_ocean_spectrum_pars.size,
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
    m_developed_spectrum_texture = m_render_engine->create_texture(
        developed_spectrum_texture_desc,
        L"Texture:Ocean:Developed_Spectrum");

    Buffer_Desc initial_spectrum_params_buffer = {
        .size = sizeof(Ocean_Spectrum_Ocean_Parameters),
        .heap_type = D3D12_HEAP_TYPE_DEFAULT,
        .usage = Resource_Usage::Read_Only
    };
    m_initial_spectrum_ocean_params_buffer = m_render_engine->create_buffer(
        initial_spectrum_params_buffer,
        L"Buffer:Ocean:Initial_Spectrum_Params");

    m_initial_spectrum_bindset = m_render_engine->create_bindset();
    Ocean_Initial_Spectrum_Shader_Bindset initial_spectrum_bindset_data = {
        .ocean_params_buf_idx = uint32_t(m_initial_spectrum_ocean_params_buffer.bindless_idx),
        .initial_spectrum_tex_idx = uint32_t(m_initial_spectrum_texture.bindless_idx),
        .angular_frequency_tex_idx = uint32_t(m_angular_frequency_texture.bindless_idx)
    };
    m_initial_spectrum_bindset.write_data(initial_spectrum_bindset_data);
    m_render_engine->update_bindings(m_initial_spectrum_bindset);

    m_developed_spectrum_bindset = m_render_engine->create_bindset();
}

Ocean_Calculate_Spectra_Render_Procedure::~Ocean_Calculate_Spectra_Render_Procedure()
{
    m_render_engine->destroy_bindset(m_initial_spectrum_bindset);

    m_render_engine->destroy_buffer(m_initial_spectrum_ocean_params_buffer);
    m_render_engine->destroy_texture(m_developed_spectrum_texture);
    m_render_engine->destroy_texture(m_angular_frequency_texture);
    m_render_engine->destroy_texture(m_initial_spectrum_texture);
    m_render_engine->destroy_pipeline(m_developed_spectrum_compute_pso);
    m_render_engine->destroy_shader(m_developed_spectrum_shader);
    m_render_engine->destroy_pipeline(m_initial_spectrum_compute_pso);
    m_render_engine->destroy_shader(m_initial_spectrum_shader);
    m_render_engine->destroy_pipeline(m_fft_compute_pso);
    m_render_engine->destroy_shader(m_fft_shader);
}

struct FFT_Constants
{
    uint32_t texture;
    uint32_t vertical;
    uint32_t inverse;
    uint32_t scale;
};

void Ocean_Calculate_Spectra_Render_Procedure::process(const Render_Procedure_Payload& payload)
{
    auto barrier_builder = payload.cmd->acquire_barrier_builder();
    m_time += payload.delta_time;
    payload.cmd->begin_event("Ocean_Simulation");
    initial_spectrum_pass(payload, barrier_builder);
    developed_spectrum_pass(payload, barrier_builder);
    fft_pass(payload, barrier_builder);
}

void Ocean_Calculate_Spectra_Render_Procedure::initial_spectrum_pass(const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder)
{
    auto size = m_ocean_spectrum_pars.size;
    if (m_is_dirty)
    {
        payload.cmd->begin_event("Initial_Spectrum_Computation");
        // recalculate v yu karaev spectrum omega_m values for double peaked spectrum.
        m_ocean_spectrum_pars.spectra[0].v_yu_karaev_spectrum_omega_m =
            ocean_spectrum_calculate_v_yu_karaev_spectrum_omega_m(m_ocean_spectrum_pars.spectra[0].fetch);
        m_ocean_spectrum_pars.spectra[1].v_yu_karaev_spectrum_omega_m =
            ocean_spectrum_calculate_v_yu_karaev_spectrum_omega_m(m_ocean_spectrum_pars.spectra[1].fetch);
        auto upload = payload.render_engine->upload_data(sizeof(Ocean_Spectrum_Ocean_Parameters), 0, m_initial_spectrum_ocean_params_buffer, 0);
        memcpy(upload, &m_ocean_spectrum_pars, sizeof(Ocean_Spectrum_Ocean_Parameters));

        barrier_builder.push({
            .texture = m_initial_spectrum_texture,
            .sync_before = D3D12_BARRIER_SYNC_NONE,
            .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .access_before = D3D12_BARRIER_ACCESS_NO_ACCESS,
            .access_after = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            .layout_before = D3D12_BARRIER_LAYOUT_UNDEFINED,
            .layout_after = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
            .subresources = {
                .IndexOrFirstMipLevel = 0,
                .NumMipLevels = 1,
                .FirstArraySlice = 0,
                .NumArraySlices = 1,
                .FirstPlane = 0,
                .NumPlanes = 1
            },
            .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
            });
        barrier_builder.push({
            .texture = m_angular_frequency_texture,
            .sync_before = D3D12_BARRIER_SYNC_NONE,
            .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .access_before = D3D12_BARRIER_ACCESS_NO_ACCESS,
            .access_after = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            .layout_before = D3D12_BARRIER_LAYOUT_UNDEFINED,
            .layout_after = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
            .subresources = {
                .IndexOrFirstMipLevel = 0,
                .NumMipLevels = 1,
                .FirstArraySlice = 0,
                .NumArraySlices = 1,
                .FirstPlane = 0,
                .NumPlanes = 1
            },
            .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
            });
        barrier_builder.flush();

        payload.cmd->set_bindset_compute(m_initial_spectrum_bindset);
        payload.cmd->set_pipeline_state(m_initial_spectrum_compute_pso);
        payload.cmd->dispatch_div_by_workgroups(m_initial_spectrum_compute_pso, size, size, 1);

        barrier_builder.push({
            .texture = m_initial_spectrum_texture,
            .sync_before = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .access_before = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            .access_after = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
            .layout_before = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
            .layout_after = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
            .subresources = {
                .IndexOrFirstMipLevel = 0,
                .NumMipLevels = 1,
                .FirstArraySlice = 0,
                .NumArraySlices = 1,
                .FirstPlane = 0,
                .NumPlanes = 1
            },
            .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
            });
        barrier_builder.push({
            .texture = m_angular_frequency_texture,
            .sync_before = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .access_before = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            .access_after = D3D12_BARRIER_ACCESS_SHADER_RESOURCE,
            .layout_before = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
            .layout_after = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
            .subresources = {
                .IndexOrFirstMipLevel = 0,
                .NumMipLevels = 1,
                .FirstArraySlice = 0,
                .NumArraySlices = 1,
                .FirstPlane = 0,
                .NumPlanes = 1
            },
            .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
            });

        payload.cmd->end_event();
        m_is_dirty = true;
    }
}

void Ocean_Calculate_Spectra_Render_Procedure::developed_spectrum_pass(const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder)
{
    auto size = m_ocean_spectrum_pars.size;
    payload.cmd->begin_event("Developed_Spectrum_Computation");
    barrier_builder.push({
        .texture = m_developed_spectrum_texture,
        .sync_before = D3D12_BARRIER_SYNC_NONE,
        .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
        .access_before = D3D12_BARRIER_ACCESS_NO_ACCESS,
        .access_after = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
        .layout_before = D3D12_BARRIER_LAYOUT_UNDEFINED,
        .layout_after = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
        .subresources = {
            .IndexOrFirstMipLevel = 0,
            .NumMipLevels = 1,
            .FirstArraySlice = 0,
            .NumArraySlices = 1,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.flush();

    Ocean_Developed_Spectrum_Shader_Bindset developed_spectrum_bindset = {
        .initial_spectrum_tex_idx = uint32_t(m_initial_spectrum_texture.bindless_idx),
        .angular_frequency_tex_idx = uint32_t(m_angular_frequency_texture.bindless_idx),
        .developed_spectrum_tex_idx = uint32_t(m_developed_spectrum_texture.bindless_idx),
        .time = m_time,
        .size = m_ocean_spectrum_pars.size
    };
    m_developed_spectrum_bindset.write_data(developed_spectrum_bindset);
    payload.render_engine->update_bindings(m_developed_spectrum_bindset);
    payload.cmd->set_bindset_compute(m_developed_spectrum_bindset);
    payload.cmd->set_pipeline_state(m_developed_spectrum_compute_pso);
    payload.cmd->dispatch_div_by_workgroups(m_developed_spectrum_compute_pso, size, size, 1);

    barrier_builder.push({
        .texture = m_developed_spectrum_texture,
        .sync_before = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
        .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
        .access_before = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
        .access_after = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
        .layout_before = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
        .layout_after = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
        .subresources = {
            .IndexOrFirstMipLevel = 0,
            .NumMipLevels = 1,
            .FirstArraySlice = 0,
            .NumArraySlices = 1,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.flush();
    payload.cmd->end_event();
}

void Ocean_Calculate_Spectra_Render_Procedure::fft_pass(const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder)
{
    auto size = m_ocean_spectrum_pars.size;
    payload.cmd->begin_event("FFT");
    payload.cmd->set_pipeline_state(m_fft_compute_pso);
    FFT_Constants fft_constants = {
        .texture = uint32_t(m_developed_spectrum_texture.bindless_idx),
        .vertical = false,
        .inverse = true,
        .scale = true
    };
    payload.cmd->set_constants_compute(3, &fft_constants, 0);
    payload.cmd->dispatch(1, size, 1);
    barrier_builder.push({
        .texture = m_developed_spectrum_texture,
        .sync_before = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
        .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
        .access_before = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
        .access_after = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
        .layout_before = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
        .layout_after = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
        .subresources = {
            .IndexOrFirstMipLevel = 0,
            .NumMipLevels = 1,
            .FirstArraySlice = 0,
            .NumArraySlices = 1,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.flush();

    fft_constants.vertical = true;
    payload.cmd->set_constants_compute(3, &fft_constants, 0);
    payload.cmd->dispatch(1, size, 1);
    barrier_builder.push({
        .texture = m_developed_spectrum_texture,
        .sync_before = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
        .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
        .access_before = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
        .access_after = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
        .layout_before = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
        .layout_after = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
        .subresources = {
            .IndexOrFirstMipLevel = 0,
            .NumMipLevels = 1,
            .FirstArraySlice = 0,
            .NumArraySlices = 1,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.flush();

    {
        fft_constants.scale = false;
        fft_constants.inverse = false;
        fft_constants.vertical = false;
        payload.cmd->set_constants_compute(3, &fft_constants, 0);
        payload.cmd->dispatch(1, size, 1);
        barrier_builder.push({
            .texture = m_developed_spectrum_texture,
            .sync_before = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .access_before = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            .access_after = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            .layout_before = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
            .layout_after = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
            .subresources = {
                .IndexOrFirstMipLevel = 0,
                .NumMipLevels = 1,
                .FirstArraySlice = 0,
                .NumArraySlices = 1,
                .FirstPlane = 0,
                .NumPlanes = 1
            },
            .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
            });
        barrier_builder.flush();

        fft_constants.vertical = true;
        payload.cmd->set_constants_compute(3, &fft_constants, 0);
        payload.cmd->dispatch(1, size, 1);
        barrier_builder.push({
            .texture = m_developed_spectrum_texture,
            .sync_before = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .sync_after = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .access_before = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            .access_after = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            .layout_before = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
            .layout_after = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
            .subresources = {
                .IndexOrFirstMipLevel = 0,
                .NumMipLevels = 1,
                .FirstArraySlice = 0,
                .NumArraySlices = 1,
                .FirstPlane = 0,
                .NumPlanes = 1
            },
            .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
            });
        barrier_builder.flush();
    }

    payload.cmd->end_event();

    payload.cmd->end_event();
}

Ocean_Tile_Render_Procedure::Ocean_Tile_Render_Procedure()
    : Render_Procedure("Ocean_Tile_Render")
{}

void Ocean_Tile_Render_Procedure::process(const Render_Procedure_Payload& payload)
{
    payload;
}
}
