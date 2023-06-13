#pragma once

#include "owge_render_engine/render_procedure/render_procedure.hpp"
#include "owge_render_engine/resource_allocator.hpp"
#include "owge_render_engine/command_allocator.hpp"

#include <owge_d3d12_base/d3d12_ctx.hpp>
#include <owge_d3d12_base/d3d12_util.hpp>
#include <owge_d3d12_base/d3d12_swapchain.hpp>

#include <memory>
#include <vector>

namespace owge
{
static constexpr uint32_t MAX_CONCURRENT_GPU_FRAMES = 2;
static constexpr uint32_t MAX_SWAPCHAIN_BUFFERS = MAX_CONCURRENT_GPU_FRAMES + 1;

struct Render_Engine_Settings
{
    bool nvperf_enabled;
    bool nvperf_lock_clocks_to_rated_tdp;
};

struct Render_Engine_Frame_Context
{
    std::unique_ptr<Command_Allocator> direct_queue_cmd_alloc;
    Com_Ptr<ID3D12Fence1> direct_queue_fence;
    uint64_t frame_number;
};

class Barrier_Builder;

class Render_Engine
{
public:
    Render_Engine(HWND hwnd, const D3D12_Context_Settings& d3d12_context_settings,
        const Render_Engine_Settings& render_engine_settings);
    ~Render_Engine();

    // Delete special member functions. An instance of this can't be copied nor moved.
    Render_Engine(const Render_Engine&) = delete;
    Render_Engine(Render_Engine&&) = delete;
    Render_Engine& operator=(const Render_Engine&) = delete;
    Render_Engine& operator=(Render_Engine&&) = delete;

    void add_procedure(Render_Procedure* proc);
    void render();

    Buffer_Handle create_buffer(const Buffer_Desc& desc);
    Texture_Handle create_texture(const Texture_Desc& desc);
    Pipeline_Handle create_pipeline(const Graphics_Pipeline_Desc& desc);
    Pipeline_Handle create_pipeline(const Compute_Pipeline_Desc& desc);

private:
    D3D12_Context m_ctx;
    Render_Engine_Settings m_settings;
    bool m_nvperf_active;

    std::unique_ptr<Descriptor_Allocator> m_cbv_srv_uav_descriptor_allocator;
    std::unique_ptr<Descriptor_Allocator> m_sampler_descriptor_allocator;
    Com_Ptr<ID3D12DescriptorHeap> m_rtv_descriptor_heap;
    std::unique_ptr<Descriptor_Allocator> m_rtv_descriptor_allocator;
    Com_Ptr<ID3D12DescriptorHeap> m_dsv_descriptor_heap;
    std::unique_ptr<Descriptor_Allocator> m_dsv_descriptor_allocator;

    std::unique_ptr<D3D12_Swapchain> m_swapchain;
    std::vector<Render_Procedure*> m_procedures;

    Resource_Allocator<Buffer> m_buffers;
    Resource_Allocator<Texture> m_textures;
    Resource_Allocator<Pipeline> m_pipelines;

    uint64_t m_current_frame = 0;
    uint32_t m_current_frame_index = 0;
    std::array<Render_Engine_Frame_Context, MAX_CONCURRENT_GPU_FRAMES> m_frame_contexts;
};
}
