#include "owge_render_engine/render_procedure/swapchain_pass.hpp"
#include "owge_render_engine/command_list.hpp"

#include <owge_d3d12_base/d3d12_swapchain.hpp>

namespace owge
{
Swapchain_Pass_Render_Procedure::Swapchain_Pass_Render_Procedure(
    const Swapchain_Pass_Settings& settings)
    : Render_Procedure("Swapchain_Pass"), m_settings(settings)
{}

void Swapchain_Pass_Render_Procedure::add_subprocedure(Render_Procedure* sub_procedure)
{
    m_sub_procedures.push_back(sub_procedure);
}

void Swapchain_Pass_Render_Procedure::process(const Render_Procedure_Payload& payload)
{
    auto barrier_builder = payload.cmd->acquire_barrier_builder();
    barrier_builder.push({
        .swapchain = payload.swapchain,
        .sync_before = D3D12_BARRIER_SYNC_NONE,
        .sync_after = D3D12_BARRIER_SYNC_RENDER_TARGET,
        .access_before = D3D12_BARRIER_ACCESS_NO_ACCESS,
        .access_after = D3D12_BARRIER_ACCESS_RENDER_TARGET,
        .layout_before = D3D12_BARRIER_LAYOUT_UNDEFINED,
        .layout_after = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
        .subresources = {
            .IndexOrFirstMipLevel = 0xFFFFFFFF,
            .NumMipLevels = 0,
            .FirstArraySlice = 0,
            .NumArraySlices = 0,
            .FirstPlane = 0,
            .NumPlanes = 0
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE // Maybe DISCARD?
        });
    barrier_builder.flush();

    payload.cmd->clear_render_target(payload.swapchain, m_settings.clear_color);
    payload.cmd->set_render_target_swapchain(payload.swapchain, {});

    for (auto subproc : m_sub_procedures)
    {
        subproc->process(payload);
    }

    barrier_builder.push({
        .swapchain = payload.swapchain,
        .sync_before = D3D12_BARRIER_SYNC_RENDER_TARGET,
        .sync_after = D3D12_BARRIER_SYNC_NONE,
        .access_before = D3D12_BARRIER_ACCESS_RENDER_TARGET,
        .access_after = D3D12_BARRIER_ACCESS_NO_ACCESS,
        .layout_before = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
        .layout_after = D3D12_BARRIER_LAYOUT_PRESENT,
        .subresources = {
            .IndexOrFirstMipLevel = 0xFFFFFFFF,
            .NumMipLevels = 0,
            .FirstArraySlice = 0,
            .NumArraySlices = 0,
            .FirstPlane = 0,
            .NumPlanes = 0
        },
        .flags = D3D12_TEXTURE_BARRIER_FLAG_NONE // Maybe DISCARD?
        });
    barrier_builder.flush();
}
}
