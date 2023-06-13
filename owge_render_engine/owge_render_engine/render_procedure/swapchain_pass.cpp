#include "owge_render_engine/render_procedure/swapchain_pass.hpp"

#include <owge_d3d12_base/d3d12_swapchain.hpp>

namespace owge
{
Swapchain_Pass::Swapchain_Pass(const Swapchain_Pass_Settings& settings)
    : Render_Procedure("Swapchain_Pass"), m_settings(settings)
{}

void Swapchain_Pass::process(const Render_Procedure_Payload& payload)
{
    auto swapchain_res = payload.swapchain->get_acquired_resources();
    D3D12_TEXTURE_BARRIER swapchain_barrier = {
        .SyncBefore = D3D12_BARRIER_SYNC_NONE,
        .SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET,
        .AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
        .AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET,
        .LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED,
        .LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
        .pResource = swapchain_res.buffer,
        .Subresources = {
            .IndexOrFirstMipLevel = 0xFFFFFFFF,
            .NumMipLevels = 0,
            .FirstArraySlice = 0,
            .NumArraySlices = 0,
            .FirstPlane = 0,
            .NumPlanes = 0
        },
        .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE // Maybe DISCARD?
    };
    D3D12_BARRIER_GROUP swapchain_barrier_group = {
        .Type = D3D12_BARRIER_TYPE_TEXTURE,
        .NumBarriers = 1,
        .pTextureBarriers = &swapchain_barrier
    };

    payload.cmd->Barrier(1, &swapchain_barrier_group);
    payload.cmd->ClearRenderTargetView(swapchain_res.rtv_descriptor, m_settings.clear_color, 0, nullptr);
    payload.cmd->OMSetRenderTargets(1, &swapchain_res.rtv_descriptor, FALSE, nullptr);
    for (auto subproc : m_sub_procedures)
    {
        subproc->process(payload);
    }

    swapchain_barrier.SyncBefore = D3D12_BARRIER_SYNC_RENDER_TARGET;
    swapchain_barrier.SyncAfter = D3D12_BARRIER_SYNC_ALL;
    swapchain_barrier.AccessBefore = D3D12_BARRIER_ACCESS_RENDER_TARGET;
    swapchain_barrier.AccessAfter = D3D12_BARRIER_ACCESS_COMMON;
    swapchain_barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    swapchain_barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;
    payload.cmd->Barrier(1, &swapchain_barrier_group);
}
}
