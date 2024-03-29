#pragma once

#include "owge_render_engine/render_procedure/render_procedure.hpp"
#include "owge_render_engine/resource_allocator.hpp"
#include "owge_render_engine/resource_manager.hpp"
#include "owge_render_engine/command_allocator.hpp"
#include "owge_render_engine/bindless.hpp"
#include "owge_render_engine/staging_buffer_allocator.hpp"

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
    ID3D12GraphicsCommandList7* upload_cmd;
    std::unique_ptr<Staging_Buffer_Allocator> staging_buffer_allocator;
};

struct Staged_Upload
{
    ID3D12Resource* src;
    uint64_t src_offset;
    ID3D12Resource* dst;
    uint64_t dst_offset;
    uint64_t size;
};

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
    void render(float delta_time);

    [[nodiscard]] void* upload_data(uint64_t size, uint64_t align, Buffer_Handle dst, uint64_t dst_offset);
    void copy_and_upload_data(uint64_t size, uint64_t align, Buffer_Handle dst, uint64_t dst_offset, void* data);
    void update_bindings(const Bindset& bindset);

    [[nodiscard]] Buffer_Handle create_buffer(const Buffer_Desc& desc, const wchar_t* name = nullptr);
    [[nodiscard]] Texture_Handle create_texture(const Texture_Desc& desc, const wchar_t* name = nullptr);
    [[nodiscard]] Shader_Handle create_shader(const Shader_Desc& desc);
    [[nodiscard]] Pipeline_Handle create_pipeline(const Graphics_Pipeline_Desc& desc, const wchar_t* name = nullptr);
    [[nodiscard]] Pipeline_Handle create_pipeline(const Compute_Pipeline_Desc& desc, const wchar_t* name = nullptr);
    [[nodiscard]] Sampler_Handle create_sampler(const Sampler_Desc& desc);
    [[nodiscard]] Bindset create_bindset();

    void destroy_buffer(Buffer_Handle handle);
    void destroy_texture(Texture_Handle handle);
    void destroy_shader(Shader_Handle handle);
    void destroy_pipeline(Pipeline_Handle handle);
    void destroy_sampler(Sampler_Handle handle);
    void destroy_bindset(const Bindset& bindset);
    void destroy_d3d12_resource_deferred(ID3D12Resource* resource);

    [[nodiscard]] const Buffer& get_buffer(Buffer_Handle handle) const;
    [[nodiscard]] Buffer& get_buffer(Buffer_Handle handle);
    [[nodiscard]] const Texture& get_texture(Texture_Handle handle) const;
    [[nodiscard]] Texture& get_texture(Texture_Handle handle);
    [[nodiscard]] const Pipeline& get_pipeline(Pipeline_Handle handle) const;
    [[nodiscard]] Pipeline& get_pipeline(Pipeline_Handle handle);
    [[nodiscard]] const Shader& get_shader(Shader_Handle handle) const;
    [[nodiscard]] Shader& get_shader(Shader_Handle handle);

    [[nodiscard]] D3D12_CPU_DESCRIPTOR_HANDLE get_cpu_descriptor_from_texture(Texture_Handle handle) const;
    [[nodiscard]] const D3D12_Context* get_context() const
    {
        return &m_ctx;
    }

private:
    void empty_deletion_queues(uint64_t frame);

private:
    D3D12_Context m_ctx;
    Render_Engine_Settings m_settings;
    bool m_nvperf_active;

    std::unique_ptr<Resource_Manager> m_resource_manager;

    std::unique_ptr<D3D12_Swapchain> m_swapchain;
    std::vector<Render_Procedure*> m_procedures;

    uint64_t m_current_frame = 0;
    uint32_t m_current_frame_index = 0;
    std::array<Render_Engine_Frame_Context, MAX_CONCURRENT_GPU_FRAMES> m_frame_contexts;

    std::unique_ptr<Bindset_Allocator> m_bindset_allocator;
    std::unique_ptr<Bindset_Stager> m_bindset_stager;

    std::vector<Staged_Upload> m_staged_uploads;

    template<typename T>
    struct Deletion_Queue_Resource
    {
        T resource;
        uint64_t frame;
    };
    std::vector<Deletion_Queue_Resource<Bindset>> m_bindset_deletion_queue;
};
}
