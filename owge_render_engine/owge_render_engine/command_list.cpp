#include "owge_render_engine/command_list.hpp"
#include "owge_render_engine/render_engine.hpp"
#include "owge_render_engine/bindless.hpp"

#include <owge_d3d12_base/d3d12_swapchain.hpp>

#if OWGE_USE_WIN_PIX_EVENT_RUNTIME
#include <WinPixEventRuntime/pix3.h>
#endif // OWGE_USE_WIN_PIX_EVENT_RUNTIME

#include <array>

namespace owge
{
Barrier_Builder::Barrier_Builder(Render_Engine* render_engine, ID3D12GraphicsCommandList7* cmd)
    : m_render_engine(render_engine), m_cmd(cmd)
{}

void Barrier_Builder::push(const Texture_Barrier& barrier)
{
    if (barrier.texture.is_null_handle())
    {
        auto swapchain_resources = barrier.swapchain->get_acquired_resources();
        m_texture_barriers.push_back({
            .SyncBefore = barrier.sync_before,
            .SyncAfter = barrier.sync_after,
            .AccessBefore = barrier.access_before,
            .AccessAfter = barrier.access_after,
            .LayoutBefore = barrier.layout_before,
            .LayoutAfter = barrier.layout_after,
            .pResource = swapchain_resources.buffer,
            .Subresources = barrier.subresources,
            .Flags = barrier.flags
            });
    }
    else
    {
        auto& texture = m_render_engine->get_texture(barrier.texture);
        m_texture_barriers.push_back({
            .SyncBefore = barrier.sync_before,
            .SyncAfter = barrier.sync_after,
            .AccessBefore = barrier.access_before,
            .AccessAfter = barrier.access_after,
            .LayoutBefore = barrier.layout_before,
            .LayoutAfter = barrier.layout_after,
            .pResource = texture.resource,
            .Subresources = barrier.subresources,
            .Flags = barrier.flags
            });
    }
}

void Barrier_Builder::push(const Buffer_Barrier& barrier)
{
    auto& buffer = m_render_engine->get_buffer(barrier.buffer);
    m_buffer_barriers.push_back({
        .SyncBefore = barrier.sync_before,
        .SyncAfter = barrier.sync_after,
        .AccessBefore = barrier.access_before,
        .AccessAfter = barrier.access_after,
        .pResource = buffer.resource,
        .Offset = 0,
        .Size = ~0ull
        });
}

void Barrier_Builder::push(const Memory_Barrier& barrier)
{
    m_global_barriers.push_back({
        .SyncBefore = barrier.sync_before,
        .SyncAfter = barrier.sync_after,
        .AccessBefore = barrier.access_before,
        .AccessAfter = barrier.access_after
        });
}

void Barrier_Builder::flush()
{
    uint32_t barrier_group_count = 0;
    std::array<D3D12_BARRIER_GROUP, 3> barrier_groups = {};
    if (m_texture_barriers.size() > 0)
    {
        auto& barrier_group = barrier_groups[barrier_group_count];
        barrier_group.Type = D3D12_BARRIER_TYPE_TEXTURE;
        barrier_group.NumBarriers = uint32_t(m_texture_barriers.size());
        barrier_group.pTextureBarriers = m_texture_barriers.data();
        barrier_group_count += 1;
    }
    if (m_buffer_barriers.size() > 0)
    {
        auto& barrier_group = barrier_groups[barrier_group_count];
        barrier_group.Type = D3D12_BARRIER_TYPE_BUFFER;
        barrier_group.NumBarriers = uint32_t(m_buffer_barriers.size());
        barrier_group.pBufferBarriers = m_buffer_barriers.data();
        barrier_group_count += 1;
    }
    if (m_global_barriers.size() > 0)
    {
        auto& barrier_group = barrier_groups[barrier_group_count];
        barrier_group.Type = D3D12_BARRIER_TYPE_GLOBAL;
        barrier_group.NumBarriers = uint32_t(m_global_barriers.size());
        barrier_group.pGlobalBarriers = m_global_barriers.data();
        barrier_group_count += 1;
    }
    m_cmd->Barrier(barrier_group_count, barrier_groups.data());
    m_texture_barriers.clear();
    m_buffer_barriers.clear();
    m_global_barriers.clear();
}

Command_List::Command_List(Render_Engine* render_engine, ID3D12GraphicsCommandList7* cmd)
    : m_render_engine(render_engine), m_cmd(cmd)
{}

Barrier_Builder Command_List::acquire_barrier_builder()
{
    return Barrier_Builder(m_render_engine, m_cmd);
}

void Command_List::clear_depth_stencil(Texture_Handle texture, D3D12_CLEAR_FLAGS flags, float depth, uint8_t stencil)
{
    m_cmd->ClearDepthStencilView(m_render_engine->get_cpu_descriptor_from_texture(texture), flags, depth, stencil, 0, nullptr);
}

void Command_List::clear_render_target(D3D12_Swapchain* swapchain, float clear_color[4])
{
    auto swapchain_resources = swapchain->get_acquired_resources();
    m_cmd->ClearRenderTargetView(swapchain_resources.rtv_descriptor, clear_color, 0, nullptr);
}

void Command_List::clear_render_target(Texture_Handle texture, float clear_color[4])
{
    m_cmd->ClearRenderTargetView(m_render_engine->get_cpu_descriptor_from_texture(texture), clear_color, 0, nullptr);
}

void Command_List::dispatch(uint32_t x, uint32_t y, uint32_t z)
{
    m_cmd->Dispatch(x, y, z);
}

void Command_List::dispatch_div_by_workgroups(Pipeline_Handle pso, uint32_t x, uint32_t y, uint32_t z)
{
    auto& pipeline = m_render_engine->get_pipeline(pso);
    m_cmd->Dispatch(x / pipeline.workgroups_x, y / pipeline.workgroups_y, z / pipeline.workgroups_z);
}

void Command_List::dispatch_mesh(uint32_t x, uint32_t y, uint32_t z)
{
    m_cmd->DispatchMesh(x, y, z);
}

void Command_List::dispatch_mesh_div_by_workgroups(Pipeline_Handle pso, uint32_t x, uint32_t y, uint32_t z)
{
    auto& pipeline = m_render_engine->get_pipeline(pso);
    m_cmd->DispatchMesh(x / pipeline.workgroups_x, y / pipeline.workgroups_y, z / pipeline.workgroups_z);
}

void Command_List::draw(uint32_t vertex_count, uint32_t vertex_offset,
    uint32_t instance_count, uint32_t instance_offset)
{
    m_cmd->DrawInstanced(vertex_count, instance_count, vertex_offset, instance_offset);
}

void Command_List::draw_indexed(uint32_t index_count, uint32_t index_offset,
    uint32_t instance_count, uint32_t instance_offset, uint32_t base_vertex)
{
    m_cmd->DrawIndexedInstanced(index_count, instance_count, index_offset, base_vertex, instance_offset);
}

void Command_List::set_bindset_compute(const Bindset& bindset, uint32_t first_element)
{
    auto& alloc = bindset.allocation;
    m_cmd->SetComputeRoot32BitConstants(0, 2, &alloc, first_element);
}

void Command_List::set_bindset_graphics(const Bindset& bindset, uint32_t first_element)
{
    auto& alloc = bindset.allocation;
    m_cmd->SetGraphicsRoot32BitConstants(0, 2, &alloc, first_element);
}

void Command_List::set_constants_compute(uint32_t count, void* constants, uint32_t first_constant)
{
    m_cmd->SetComputeRoot32BitConstants(0, count, constants, first_constant);
}

void Command_List::set_constants_graphics(uint32_t count, void* constants, uint32_t first_constant)
{
    m_cmd->SetGraphicsRoot32BitConstants(0, count, constants, first_constant);
}

void Command_List::set_pipeline_state(Pipeline_Handle handle)
{
    auto& pipeline = m_render_engine->get_pipeline(handle);
    m_cmd->SetPipelineState(pipeline.pso);
}

void Command_List::set_render_targets(std::span<Texture_Handle> textures, Texture_Handle depth_stencil)
{
    if (!depth_stencil.is_null_handle())
    {
        uint32_t render_target_count = 0;
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> render_targets;
        for (auto texture : textures)
        {
            render_targets[render_target_count] = m_render_engine->get_cpu_descriptor_from_texture(texture);
            render_target_count += 1;
        }
        D3D12_CPU_DESCRIPTOR_HANDLE ds_cpu_handle = m_render_engine->get_cpu_descriptor_from_texture(depth_stencil);
        m_cmd->OMSetRenderTargets(render_target_count, render_targets.data(), false, &ds_cpu_handle);
    }
    else
    {
        uint32_t render_target_count = 0;
        std::array<D3D12_CPU_DESCRIPTOR_HANDLE, 8> render_targets;
        for (auto texture : textures)
        {
            render_targets[render_target_count] = m_render_engine->get_cpu_descriptor_from_texture(texture);
            render_target_count += 1;
        }
        m_cmd->OMSetRenderTargets(render_target_count, render_targets.data(), false, nullptr);
    }
}

void Command_List::set_render_target_swapchain(D3D12_Swapchain* swapchain, Texture_Handle depth_stencil)
{
    if (!depth_stencil.is_null_handle())
    {
        auto swapchain_cpu_handle = swapchain->get_acquired_resources().rtv_descriptor;
        D3D12_CPU_DESCRIPTOR_HANDLE ds_cpu_handle = m_render_engine->get_cpu_descriptor_from_texture(depth_stencil);
        m_cmd->OMSetRenderTargets(1, &swapchain_cpu_handle, 0, &ds_cpu_handle);
    }
    else
    {
        auto swapchain_cpu_handle = swapchain->get_acquired_resources().rtv_descriptor;
        m_cmd->OMSetRenderTargets(1, &swapchain_cpu_handle, 0, nullptr);
    }
}

void Command_List::begin_event([[maybe_unused]] const char* message)
{
#if OWGE_USE_WIN_PIX_EVENT_RUNTIME
    PIXBeginEvent(m_cmd, PIX_COLOR_INDEX(m_event_index), message);
#endif // OWGE_USE_WIN_PIX_EVENT_RUNTIME
}

void Command_List::end_event()
{
#if OWGE_USE_WIN_PIX_EVENT_RUNTIME
    PIXEndEvent(m_cmd);
    m_event_index += 1;
#endif // OWGE_USE_WIN_PIX_EVENT_RUNTIME
}

void Command_List::set_marker([[maybe_unused]] const char* message)
{
#if OWGE_USE_WIN_PIX_EVENT_RUNTIME
    PIXSetMarker(m_cmd, PIX_COLOR_INDEX(m_marker_index), message);
    m_marker_index += 1;
#endif // OWGE_USE_WIN_PIX_EVENT_RUNTIME
}
}
