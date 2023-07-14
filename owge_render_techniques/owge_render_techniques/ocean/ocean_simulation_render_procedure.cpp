#include "owge_render_techniques/ocean/ocean_simulation_render_procedure.hpp"
#include "owge_render_techniques/ocean/ocean_render_resources.hpp"
#include "owge_render_techniques/ocean/ocean_settings.hpp"

#include <owge_render_engine/command_list.hpp>
#include <owge_render_engine/render_engine.hpp>

namespace owge
{
Ocean_Simulation_Render_Procedure::Ocean_Simulation_Render_Procedure(
    Ocean_Settings* settings, Ocean_Simulation_Render_Resources* resources)
    : Render_Procedure("Ocean_Simulation"), m_settings(settings), m_resources(resources)
{}

void Ocean_Simulation_Render_Procedure::process(const Render_Procedure_Payload& payload)
{
    auto barrier_builder = payload.cmd->acquire_barrier_builder();
    m_time += payload.delta_time;
    process_initial_spectrum(payload, barrier_builder);
    process_developed_spectrum(payload, barrier_builder);
    process_ffts(payload, barrier_builder);
}

float oceanography_calculate_v_yu_karaev_spectrum_omega_m(float fetch)
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

void Ocean_Simulation_Render_Procedure::process_initial_spectrum(
    const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder)
{
    if (!m_settings->recompute_initial_spectrum)
    {
        return;
    }
    payload.cmd->begin_event("Initial_Spectrum_Computation");

    Ocean_Initial_Spectrum_Shader_Bindset initial_spectrum_bindset_data = {
        .ocean_params_buf_idx = uint32_t(m_resources->initial_spectrum_ocean_params_buffer.bindless_idx),
        .initial_spectrum_tex_idx = uint32_t(m_resources->initial_spectrum_texture.bindless_idx),
        .angular_frequency_tex_idx = uint32_t(m_resources->angular_frequency_texture.bindless_idx)
    };
    m_resources->initial_spectrum_bindset.write_data(initial_spectrum_bindset_data);
    payload.render_engine->update_bindings(m_resources->initial_spectrum_bindset);

    Ocean_Simulation_Initial_Spectrum_Parameter_Buffer initial_spectrum_parameters = {
        .size = m_settings->size,
        .length_scale = m_settings->length_scales[0],
        .gravity = m_settings->gravity,
        .ocean_depth = m_settings->ocean_depth,
        .spectra = {
            {
                .wind_speed = m_settings->local_spectrum.wind_speed,
                .fetch = m_settings->local_spectrum.fetch,
                .v_yu_karaev_spectrum_omega_m =
                    oceanography_calculate_v_yu_karaev_spectrum_omega_m(m_settings->local_spectrum.fetch)
            },
            {
                .wind_speed = m_settings->swell_spectrum.wind_speed,
                .fetch = m_settings->swell_spectrum.fetch,
                .v_yu_karaev_spectrum_omega_m =
                    oceanography_calculate_v_yu_karaev_spectrum_omega_m(m_settings->swell_spectrum.fetch)
            }
        }
    };

    auto size = m_settings->size;
    auto upload = payload.render_engine->upload_data(
        sizeof(Ocean_Simulation_Initial_Spectrum_Parameter_Buffer), 0, m_resources->initial_spectrum_ocean_params_buffer, 0);
    memcpy(upload, &initial_spectrum_parameters, sizeof(Ocean_Simulation_Initial_Spectrum_Parameter_Buffer));

    barrier_builder.push({
        .texture = m_resources->initial_spectrum_texture,
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
            .NumArraySlices = m_settings->cascade_count,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.push({
        .texture = m_resources->angular_frequency_texture,
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
            .NumArraySlices = m_settings->cascade_count,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.flush();

    payload.cmd->set_bindset_compute(m_resources->initial_spectrum_bindset);
    payload.cmd->set_pipeline_state(m_resources->initial_spectrum_pso);
    payload.cmd->dispatch_div_by_workgroups(m_resources->initial_spectrum_pso, size, size, 1);

    barrier_builder.push({
        .texture = m_resources->initial_spectrum_texture,
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
            .NumArraySlices = m_settings->cascade_count,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.push({
        .texture = m_resources->angular_frequency_texture,
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
            .NumArraySlices = m_settings->cascade_count,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });

    payload.cmd->end_event();
}

struct Ocean_Developed_Spectrum_Shader_Bindset
{
    uint32_t initial_spectrum_tex_idx;
    uint32_t angular_frequency_tex_idx;
    uint32_t developed_spectrum_tex_idx;
    float time;
    uint32_t size;
};

void Ocean_Simulation_Render_Procedure::process_developed_spectrum(
    const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder)
{
    payload.cmd->begin_event("Developed_Spectrum_Computation");

    auto size = m_settings->size;
    barrier_builder.push({
        .texture = m_resources->developed_spectrum_texture,
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
            .NumArraySlices = m_settings->cascade_count,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.flush();

    Ocean_Developed_Spectrum_Shader_Bindset developed_spectrum_bindset = {
        .initial_spectrum_tex_idx = uint32_t(m_resources->initial_spectrum_texture.bindless_idx),
        .angular_frequency_tex_idx = uint32_t(m_resources->angular_frequency_texture.bindless_idx),
        .developed_spectrum_tex_idx = uint32_t(m_resources->developed_spectrum_texture.bindless_idx),
        .time = m_time,
        .size = size
    };
    m_resources->developed_spectrum_bindset.write_data(developed_spectrum_bindset);
    payload.render_engine->update_bindings(m_resources->developed_spectrum_bindset);
    payload.cmd->set_bindset_compute(m_resources->developed_spectrum_bindset);
    payload.cmd->set_pipeline_state(m_resources->developed_spectrum_pso);
    payload.cmd->dispatch_div_by_workgroups(m_resources->developed_spectrum_pso, size, size, 1);

    barrier_builder.push({
        .texture = m_resources->developed_spectrum_texture,
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
            .NumArraySlices = m_settings->cascade_count,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.flush();

    payload.cmd->end_event();
}

struct FFT_Constants
{
    uint32_t texture;
    uint32_t vertical;
    uint32_t inverse;
};

void Ocean_Simulation_Render_Procedure::process_ffts(
    const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder)
{
    payload.cmd->begin_event("FFT");

    auto size = m_settings->size;
    switch (size)
    {
    case 128:
        payload.cmd->set_pipeline_state(m_resources->fft_128_pso);
        break;
    case 256:
        payload.cmd->set_pipeline_state(m_resources->fft_256_pso);
        break;
    case 512:
        payload.cmd->set_pipeline_state(m_resources->fft_512_pso);
        break;
    default:
        std::unreachable();
        break;
    }
    FFT_Constants fft_constants = {
        .texture = uint32_t(m_resources->developed_spectrum_texture.bindless_idx),
        .vertical = false,
        .inverse = true
    };
    payload.cmd->set_constants_compute(sizeof(FFT_Constants) / sizeof(uint32_t), &fft_constants, 0);
    payload.cmd->dispatch(1, size, 1);
    barrier_builder.push({
        .texture = m_resources->developed_spectrum_texture,
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
            .NumArraySlices = m_settings->cascade_count,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.flush();

    fft_constants.vertical = true;
    payload.cmd->set_constants_compute(sizeof(FFT_Constants) / sizeof(uint32_t), &fft_constants, 0);
    payload.cmd->dispatch(1, size, 1);
    barrier_builder.push({
        .texture = m_resources->developed_spectrum_texture,
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
            .NumArraySlices = m_settings->cascade_count,
            .FirstPlane = 0,
            .NumPlanes = 1
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        });
    barrier_builder.flush();

    payload.cmd->end_event();
}
}
