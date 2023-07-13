#pragma once

#include "owge_render_engine/resource.hpp"

#include <cstdint>
#include <include/d3d12.h>
#include <span>
#include <vector>

namespace owge
{
class Render_Engine;
class D3D12_Swapchain;
struct Bindset;

struct Texture_Barrier
{
    Texture_Handle texture;
    D3D12_Swapchain* swapchain;
    D3D12_BARRIER_SYNC sync_before;
    D3D12_BARRIER_SYNC sync_after;
    D3D12_BARRIER_ACCESS access_before;
    D3D12_BARRIER_ACCESS access_after;
    D3D12_BARRIER_LAYOUT layout_before;
    D3D12_BARRIER_LAYOUT layout_after;
    D3D12_BARRIER_SUBRESOURCE_RANGE subresources;
    D3D12_TEXTURE_BARRIER_FLAGS flags;
};

struct Buffer_Barrier
{
    Buffer_Handle buffer;
    D3D12_BARRIER_SYNC sync_before;
    D3D12_BARRIER_SYNC sync_after;
    D3D12_BARRIER_ACCESS access_before;
    D3D12_BARRIER_ACCESS access_after;
};

struct Memory_Barrier
{
    D3D12_BARRIER_SYNC sync_before;
    D3D12_BARRIER_SYNC sync_after;
    D3D12_BARRIER_ACCESS access_before;
    D3D12_BARRIER_ACCESS access_after;
};

class Barrier_Builder
{
public:
    Barrier_Builder(Render_Engine* render_engine, ID3D12GraphicsCommandList7* cmd);

    void push(const Texture_Barrier& barrier);
    void push(const Buffer_Barrier& barrier);
    void push(const Memory_Barrier& barrier);
    void flush();

private:
    ID3D12GraphicsCommandList7* m_cmd;
    Render_Engine* m_render_engine;
    std::vector<D3D12_TEXTURE_BARRIER> m_texture_barriers;
    std::vector<D3D12_BUFFER_BARRIER> m_buffer_barriers;
    std::vector<D3D12_GLOBAL_BARRIER> m_global_barriers;
};

class Command_List
{
public:
    Command_List(Render_Engine* render_engine, ID3D12GraphicsCommandList7* cmd);

    [[nodiscard]] ID3D12GraphicsCommandList7* d3d12_cmd()
    {
        return m_cmd;
    }
    [[nodiscard]] Barrier_Builder acquire_barrier_builder();

    void clear_depth_stencil(Texture_Handle texture, D3D12_CLEAR_FLAGS flags, float depth, uint8_t stencil);
    void clear_render_target(D3D12_Swapchain* swapchain, float clear_color[4]);
    void clear_render_target(Texture_Handle texture, float clear_color[4]);
    void dispatch(uint32_t x, uint32_t y, uint32_t z);
    void dispatch_div_by_workgroups(Pipeline_Handle pso, uint32_t x, uint32_t y, uint32_t z);
    void dispatch_mesh(uint32_t x, uint32_t y, uint32_t z);
    void dispatch_mesh_div_by_workgroups(Pipeline_Handle pso, uint32_t x, uint32_t y, uint32_t z);
    void draw(uint32_t vertex_count, uint32_t vertex_offset,
        uint32_t instance_count, uint32_t instance_offset);
    void draw_indexed(uint32_t index_count, uint32_t index_offset,
        uint32_t instance_count, uint32_t instance_offset, uint32_t base_vertex);
    void set_bindset_compute(const Bindset& bindset);
    void set_bindset_graphics(const Bindset& bindset);
    void set_constants_compute(uint32_t count, void* constants, uint32_t first_constant);
    void set_constants_graphics(uint32_t count, void* constants, uint32_t first_constant);
    void set_pipeline_state(Pipeline_Handle handle);
    void set_render_targets(std::span<Texture_Handle> textures, Texture_Handle depth_stencil);
    void set_render_target_swapchain(D3D12_Swapchain* swapchain, Texture_Handle depth_stencil);

    void begin_event(const char* message);
    void end_event();
    void set_marker(const char* message);

private:
    Render_Engine* m_render_engine;
    ID3D12GraphicsCommandList7* m_cmd;
    uint8_t m_event_index = 0;
    uint8_t m_marker_index = 0;
};
}
