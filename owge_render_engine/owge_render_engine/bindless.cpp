#include "owge_render_engine/bindless.hpp"
#include "owge_render_engine/resource.hpp"
#include "owge_render_engine/render_engine.hpp"

namespace owge
{
static constexpr uint64_t MAX_BINDSETS_IN_RESOURCE = 65536;
static constexpr uint64_t BINDSET_ALLOCATOR_BUFFER_SIZE =
    sizeof(uint32_t) * MAX_BINDSET_VALUES * MAX_BINDSETS_IN_RESOURCE;

void Bindset::set_values(uint32_t index, uint32_t count, void* values)
{
    memcpy(&data[index], values, count * sizeof(uint32_t));
}

Bindset_Allocator::Bindset_Allocator(Render_Engine* render_engine)
    : m_render_engine(render_engine)
{}

Bindset Bindset_Allocator::allocate_bindset()
{
    Bindset_Allocation allocation = {};
    if (!m_freelist.empty())
    {
        allocation = m_freelist.back();
        m_freelist.pop_back();
        return Bindset(allocation);
    }
    if (m_resources.empty() || m_current_index >= MAX_BINDSETS_IN_RESOURCE)
    {
        Buffer_Desc buffer_desc = {
            .size = BINDSET_ALLOCATOR_BUFFER_SIZE,
            .heap_type = D3D12_HEAP_TYPE_DEFAULT,
            .initial_layout = D3D12_BARRIER_LAYOUT_SHADER_RESOURCE,
            .usage = Resource_Usage::Read_Only
        };
        m_resources.push_back(m_render_engine->create_buffer(buffer_desc));
    }
    allocation.offset = m_current_index * sizeof(uint32_t) * MAX_BINDSET_VALUES;
    allocation.index = m_current_index;
    allocation.srv = m_render_engine->get_buffer(m_resources.back()).srv;
    allocation.resource = m_resources.back();
    m_current_index += 1;
    return Bindset(allocation);
}

void cmd_set_bindset_graphics(ID3D12GraphicsCommandList9* cmd, const Bindset& bindset)
{
    auto& alloc = bindset.allocation;
    cmd->SetGraphicsRoot32BitConstants(0, 2, &alloc, 0);
}

void cmd_set_bindset_compute(ID3D12GraphicsCommandList9* cmd, const Bindset& bindset)
{
    auto& alloc = bindset.allocation;
    cmd->SetComputeRoot32BitConstants(0, 2, &alloc, 0);
}

void Bindset_Stager::stage_bindset(
    Render_Engine* render_engine,
    const Bindset& bindset,
    Staging_Buffer_Allocator* staging_buffer_allocator)
{
    static constexpr uint64_t ALLOC_SIZE = sizeof(uint32_t) * MAX_BINDSET_VALUES;
    auto& bindset_allocation = bindset.allocation;
    auto& bindset_allocation_buffer = render_engine->get_buffer(bindset_allocation.resource);
    auto staging_buffer_alloc = staging_buffer_allocator->allocate(ALLOC_SIZE);
    memcpy(
        &static_cast<uint8_t*>(staging_buffer_alloc.data)[staging_buffer_alloc.offset],
        bindset.data,
        ALLOC_SIZE);
    auto& staged_bindset_copy = m_bindset_staged_copies.emplace_back();
    staged_bindset_copy.src = staging_buffer_alloc.resource;
    staged_bindset_copy.src_offset = staging_buffer_alloc.offset;
    staged_bindset_copy.dst = bindset_allocation_buffer.resource;
    staged_bindset_copy.dst_offset = 0;
}

void Bindset_Stager::process(ID3D12GraphicsCommandList9* cmd)
{
    for (const auto& staged_copy : m_bindset_staged_copies)
    {
        cmd->CopyBufferRegion(
            staged_copy.dst, staged_copy.dst_offset,
            staged_copy.src, staged_copy.src_offset,
            sizeof(uint32_t) * MAX_BINDSET_VALUES);
    }

    D3D12_GLOBAL_BARRIER global_barrier = {
        .SyncBefore = D3D12_BARRIER_SYNC_NONE,
        .SyncAfter = D3D12_BARRIER_SYNC_COPY,
        .AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
        .AccessAfter = D3D12_BARRIER_ACCESS_COPY_DEST
    };
    D3D12_BARRIER_GROUP barrier_group = {
        .Type = D3D12_BARRIER_TYPE_GLOBAL,
        .pGlobalBarriers = &global_barrier
    };
    global_barrier.SyncBefore = D3D12_BARRIER_SYNC_COPY;
    global_barrier.SyncAfter = D3D12_BARRIER_SYNC_ALL_SHADING | D3D12_BARRIER_SYNC_RAYTRACING;
    global_barrier.AccessBefore = D3D12_BARRIER_ACCESS_COPY_DEST;
    global_barrier.AccessAfter = D3D12_BARRIER_ACCESS_SHADER_RESOURCE;
    cmd->Barrier(1, &barrier_group);
}
}
