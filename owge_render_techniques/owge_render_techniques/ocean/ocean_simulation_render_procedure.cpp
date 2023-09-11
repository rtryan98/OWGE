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
    process_reorder(payload, barrier_builder);
}

float oceanography_calculate_v_yu_karaev_spectrum_omega_m(float fetch)
{
    double f = fetch;
    double result = 0.61826 + 0.0000003529 * f - 0.00197508
        * sqrt(f) + (62.554 / sqrt(f)) - (290.2 / f);
    return float(result);
}

float ocean_calculate_lower_spectrum_cutoff(float length_scale)
{
    return 2.0f * XM_PI / length_scale;
}

float ocean_calculate_higher_spectrum_cutoff(uint32_t size, float length_scale)
{
    return XM_PI * float(size) / length_scale;
}

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
        .length_scales = {
            m_settings->length_scales[0],
            m_settings->length_scales[1],
            m_settings->length_scales[2],
            m_settings->length_scales[3]
        },
        .spectral_cutoffs_low = {
            std::max(
                ocean_calculate_lower_spectrum_cutoff(m_settings->length_scales[0]),
                ocean_calculate_higher_spectrum_cutoff(m_settings->size, m_settings->length_scales[1])),
            std::max(
                ocean_calculate_lower_spectrum_cutoff(m_settings->length_scales[1]),
                ocean_calculate_higher_spectrum_cutoff(m_settings->size, m_settings->length_scales[2])),
            std::max(
                ocean_calculate_lower_spectrum_cutoff(m_settings->length_scales[2]),
                ocean_calculate_higher_spectrum_cutoff(m_settings->size, m_settings->length_scales[3])),
            std::max(
                ocean_calculate_lower_spectrum_cutoff(m_settings->length_scales[3]),
                0.0f)
        },
        .spectral_cutoffs_high = {
            ocean_calculate_higher_spectrum_cutoff(m_settings->size, m_settings->length_scales[0]),
            ocean_calculate_higher_spectrum_cutoff(m_settings->size, m_settings->length_scales[1]),
            ocean_calculate_higher_spectrum_cutoff(m_settings->size, m_settings->length_scales[2]),
            ocean_calculate_higher_spectrum_cutoff(m_settings->size, m_settings->length_scales[3])
        },
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
    payload.cmd->dispatch_div_by_workgroups(
        m_resources->initial_spectrum_pso,
        size, size, m_settings->cascade_count,
        true, true, false);

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

void Ocean_Simulation_Render_Procedure::process_developed_spectrum(
    const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder)
{
    payload.cmd->begin_event("Developed_Spectrum_Computation");

    auto size = m_settings->size;

    auto textures = std::to_array({
        m_resources->packed_x_y_texture,
        m_resources->packed_z_x_dx_texture,
        m_resources->packed_y_dx_z_dx_texture,
        m_resources->packed_y_dy_z_dy_texture
        });
    for (auto texture : textures)
    {
        barrier_builder.push({
        .texture = texture,
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
    }
    barrier_builder.flush();

    Ocean_Developed_Spectrum_Shader_Bindset developed_spectrum_bindset = {
        .initial_spectrum_tex_idx = uint32_t(m_resources->initial_spectrum_texture.bindless_idx),
        .angular_frequency_tex_idx = uint32_t(m_resources->angular_frequency_texture.bindless_idx),
        .time = m_time,
        .size = size,
        .packed_spectrum_x_y_tex_idx = uint32_t(m_resources->packed_x_y_texture.bindless_idx),
        .packed_spectrum_z_x_dx_tex_idx = uint32_t(m_resources->packed_z_x_dx_texture.bindless_idx),
        .packed_spectrum_y_dx_z_dx_tex_idx = uint32_t(m_resources->packed_y_dx_z_dx_texture.bindless_idx),
        .packed_spectrum_y_dy_z_dy_tex_idx = uint32_t(m_resources->packed_y_dy_z_dy_texture.bindless_idx)
    };
    m_resources->developed_spectrum_bindset.write_data(developed_spectrum_bindset);
    payload.render_engine->update_bindings(m_resources->developed_spectrum_bindset);
    payload.cmd->set_bindset_compute(m_resources->developed_spectrum_bindset);
    payload.cmd->set_pipeline_state(m_resources->developed_spectrum_pso);
    payload.cmd->dispatch_div_by_workgroups(
        m_resources->developed_spectrum_pso,
        size, size, m_settings->cascade_count,
        true, true, false);

    for (auto texture : textures)
    {
        barrier_builder.push({
        .texture = texture,
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
    }
    barrier_builder.flush();

    payload.cmd->end_event();
}

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

    auto textures = std::to_array({
        m_resources->packed_x_y_texture,
        m_resources->packed_z_x_dx_texture,
        m_resources->packed_y_dx_z_dx_texture,
        m_resources->packed_y_dy_z_dy_texture
        });

    Texture_Barrier tex_barrier = {
        .texture = {},
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
    };

    Ocean_FFT_Constants constants = {
        .texture = 0,
        .vertical = false,
        .inverse = true
    };
    for (auto texture : textures)
    {
        tex_barrier.texture = texture;
        constants.texture = uint32_t(texture.bindless_idx);
        payload.cmd->set_constants_compute(sizeof(Ocean_FFT_Constants) / sizeof(uint32_t), &constants, 0);
        payload.cmd->dispatch(1, size, m_settings->cascade_count);
        barrier_builder.push(tex_barrier);
    }

    barrier_builder.flush();

    constants.vertical = true;
    for (auto texture : textures)
    {
        tex_barrier.texture = texture;
        constants.texture = uint32_t(texture.bindless_idx);
        payload.cmd->set_constants_compute(sizeof(Ocean_FFT_Constants) / sizeof(uint32_t), &constants, 0);
        payload.cmd->dispatch(1, size, m_settings->cascade_count);
        barrier_builder.push(tex_barrier);
    }

    payload.cmd->end_event();
}

void Ocean_Simulation_Render_Procedure::process_reorder(
    const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder)
{
    payload.cmd->begin_event("Reorder Textures + Jacobian");

    auto tex_barrier = Texture_Barrier{
        .texture = m_resources->displacement_x_y_z_texture,
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
    };
    barrier_builder.push(tex_barrier);
    tex_barrier.texture = m_resources->derivatives_texture;
    barrier_builder.push(tex_barrier);
    tex_barrier.texture = m_resources->jacobian_texture;
    barrier_builder.push(tex_barrier);
    barrier_builder.flush();

    auto size = m_settings->size;
    payload.cmd->set_bindset_compute(m_resources->texture_reorder_bindset);
    payload.cmd->set_pipeline_state(m_resources->texture_reorder_pso);
    payload.cmd->dispatch_div_by_workgroups(m_resources->texture_reorder_pso,
        size, size, m_settings->cascade_count,
        true, true, false);

    tex_barrier.sync_before = D3D12_BARRIER_SYNC_COMPUTE_SHADING;

    tex_barrier.access_before = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS;
    tex_barrier.access_after = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
    tex_barrier.layout_before = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS;
    tex_barrier.layout_after = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE;

    tex_barrier.texture = m_resources->displacement_x_y_z_texture;
    tex_barrier.sync_after = D3D12_BARRIER_SYNC_VERTEX_SHADING;
    barrier_builder.push(tex_barrier);
    tex_barrier.texture = m_resources->derivatives_texture;
    tex_barrier.sync_after = D3D12_BARRIER_SYNC_PIXEL_SHADING;
    barrier_builder.push(tex_barrier);
    tex_barrier.texture = m_resources->jacobian_texture;
    tex_barrier.sync_after = D3D12_BARRIER_SYNC_PIXEL_SHADING;
    barrier_builder.push(tex_barrier);
    barrier_builder.flush();

    payload.cmd->end_event();
}
}
