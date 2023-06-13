#include "owge_render_engine/render_procedure/ocean.hpp"
#include "owge_render_engine/render_engine.hpp"

namespace owge
{
float ocean_spectrum_calculate_v_yu_karaev_spectrum_omega_m(float fetch)
{
    double f = fetch;
    double result = 0.61826 + 0.0000003529 * f - 0.00197508
        * sqrt(f) + (62.554 / sqrt(f)) - (290.2 / f);
    return float(result);
}

Ocean_Calculate_Spectra_Render_Procedure::Ocean_Calculate_Spectra_Render_Procedure(
    Render_Engine* render_engine,
    const Ocean_Spectrum_Ocean_Parameters& ocean_spectrum_pars)
    : m_render_engine(render_engine), m_ocean_spectrum_pars(ocean_spectrum_pars)
{
    Shader_Desc initial_spectrum_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\initial_spectrum.cs.bin"
    };
    m_initial_spectrum_shader = m_render_engine->create_shader(initial_spectrum_shader_desc);
    Compute_Pipeline_Desc initial_spectrum_compute_pso_desc = {
        .cs = m_initial_spectrum_shader
    };
    m_initial_spectrum_compute_pso = m_render_engine->create_pipeline(initial_spectrum_compute_pso_desc);

    Shader_Desc developed_spectrum_shader_desc = {
        .path = ".\\res\\builtin\\shader\\ocean\\developed_spectrum.cs.bin"
    };
    m_developed_spectrum_shader = m_render_engine->create_shader(developed_spectrum_shader_desc);
    Compute_Pipeline_Desc developed_spectrum_compute_pso_desc = {
        .cs = m_developed_spectrum_shader
    };
    m_developed_spectrum_compute_pso = m_render_engine->create_pipeline(developed_spectrum_compute_pso_desc);

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
        .format = DXGI_FORMAT_R32G32_FLOAT
    };
    m_initial_spectrum_texture = m_render_engine->create_texture(initial_spectrum_texture_desc);
}

Ocean_Calculate_Spectra_Render_Procedure::~Ocean_Calculate_Spectra_Render_Procedure()
{
    m_render_engine->destroy_texture(m_initial_spectrum_texture);
    m_render_engine->destroy_pipeline(m_developed_spectrum_compute_pso);
    m_render_engine->destroy_shader(m_developed_spectrum_shader);
    m_render_engine->destroy_pipeline(m_initial_spectrum_compute_pso);
    m_render_engine->destroy_shader(m_initial_spectrum_shader);
}

void Ocean_Calculate_Spectra_Render_Procedure::process(const Render_Procedure_Payload& payload)
{
    if (m_is_dirty)
    {
        // recalculate v yu karaev spectrum omega_m values for double peaked spectrum.
        m_ocean_spectrum_pars.spectra[0].v_yu_karaev_spectrum_omega_m =
            ocean_spectrum_calculate_v_yu_karaev_spectrum_omega_m(m_ocean_spectrum_pars.spectra[0].fetch);
        m_ocean_spectrum_pars.spectra[1].v_yu_karaev_spectrum_omega_m =
            ocean_spectrum_calculate_v_yu_karaev_spectrum_omega_m(m_ocean_spectrum_pars.spectra[1].fetch);

        auto& initial_spectrum_texture = payload.render_engine->get_texture(m_initial_spectrum_texture);
        D3D12_TEXTURE_BARRIER initial_spectrum_texture_barrier = {
            .SyncBefore = D3D12_BARRIER_SYNC_NONE,
            .SyncAfter = D3D12_BARRIER_SYNC_COMPUTE_SHADING,
            .AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
            .AccessAfter = D3D12_BARRIER_ACCESS_UNORDERED_ACCESS,
            .LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED,
            .LayoutAfter = D3D12_BARRIER_LAYOUT_UNORDERED_ACCESS,
            .pResource = initial_spectrum_texture.resource,
            .Subresources = {
                .IndexOrFirstMipLevel = 0,
                .NumMipLevels = 1,
                .FirstArraySlice = 0,
                .NumArraySlices = 1,
                .FirstPlane = 0,
                .NumPlanes = 1
            },
            .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE
        };
        D3D12_BARRIER_GROUP initial_spectrum_texture_barrier_group = {
            .Type = D3D12_BARRIER_TYPE_TEXTURE,
            .NumBarriers = 1,
            .pTextureBarriers = &initial_spectrum_texture_barrier
        };
        payload.cmd->Barrier(1, &initial_spectrum_texture_barrier_group);

        auto& initial_spectrum_pso = payload.render_engine->get_pipeline(m_initial_spectrum_compute_pso);
        payload.cmd->SetPipelineState(initial_spectrum_pso.pso);
        payload.cmd->Dispatch(m_ocean_spectrum_pars.size / 32, m_ocean_spectrum_pars.size / 32, 1);
        m_is_dirty = false;
    }
}

void Ocean_Tile_Render_Procedure::process(const Render_Procedure_Payload& payload)
{
    payload;
}
}
