#pragma once

#include "owge_render_engine/render_procedure/render_procedure.hpp"
#include "owge_render_engine/resource_allocator.hpp"

#include <owge_d3d12_base/d3d12_ctx.hpp>
#include <owge_d3d12_base/d3d12_util.hpp>
#include <owge_d3d12_base/d3d12_swapchain.hpp>

#include <memory>
#include <vector>

namespace owge
{
class Render_Engine
{
public:
    Render_Engine(HWND hwnd, const D3D12_Context_Settings& d3d12_context_settings);
    ~Render_Engine();

    // Delete special member functions. An instance of this can't be copied nor moved.
    Render_Engine(const Render_Engine&) = delete;
    Render_Engine(Render_Engine&&) = delete;
    Render_Engine& operator=(const Render_Engine&) = delete;
    Render_Engine& operator=(Render_Engine&&) = delete;

    void render();

    Buffer_Handle create_buffer(const Buffer_Desc& desc);
    Texture_Handle create_texture(const Texture_Desc& desc);
    Pipeline_Handle create_pipeline(const Graphics_Pipeline_Desc& desc);
    Pipeline_Handle create_pipeline(const Compute_Pipeline_Desc& desc);

private:
    D3D12_Context m_ctx;

    std::unique_ptr<Descriptor_Allocator> m_cbv_srv_uav_descriptor_allocator;
    std::unique_ptr<Descriptor_Allocator> m_sampler_descriptor_allocator;
    Com_Ptr<ID3D12DescriptorHeap> m_rtv_descriptor_heap;
    std::unique_ptr<Descriptor_Allocator> m_rtv_descriptor_allocator;
    Com_Ptr<ID3D12DescriptorHeap> m_dsv_descriptor_heap;
    std::unique_ptr<Descriptor_Allocator> m_dsv_descriptor_allocator;

    std::unique_ptr<D3D12_Swapchain> m_swapchain;
    std::vector<std::unique_ptr<Render_Procedure>> m_procedures;

    Resource_Allocator<Buffer> m_buffers;
    Resource_Allocator<Texture> m_textures;
    Resource_Allocator<Pipeline> m_pipelines;
};
}
