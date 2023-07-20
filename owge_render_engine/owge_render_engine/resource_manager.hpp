#pragma once

#include "owge_render_engine/resource.hpp"
#include "owge_render_engine/resource_allocator.hpp"

#include "owge_d3d12_base/com_ptr.hpp"
#include "owge_d3d12_base/d3d12_util.hpp"

#include <dxcapi.h>

namespace owge
{
struct D3D12_Context;

class Resource_Manager
{
public:
    Resource_Manager(D3D12_Context* ctx);

    [[nodiscard]] Buffer_Handle create_buffer(const Buffer_Desc& desc, const wchar_t* name = nullptr);
    [[nodiscard]] Texture_Handle create_texture(const Texture_Desc& desc, const wchar_t* name = nullptr);
    [[nodiscard]] Shader_Handle create_shader(const Shader_Desc& desc);
    [[nodiscard]] Pipeline_Handle create_pipeline(const Graphics_Pipeline_Desc& desc, const wchar_t* name = nullptr);
    [[nodiscard]] Pipeline_Handle create_pipeline(const Compute_Pipeline_Desc& desc, const wchar_t* name = nullptr);
    [[nodiscard]] Sampler_Handle create_sampler(const Sampler_Desc& desc);

    void destroy_buffer(Buffer_Handle handle, uint64_t frame);
    void destroy_texture(Texture_Handle handle, uint64_t frame);
    void destroy_shader(Shader_Handle handle);
    void destroy_pipeline(Pipeline_Handle handle, uint64_t frame);
    void destroy_sampler(Sampler_Handle handle, uint64_t frame);
    void destroy_d3d12_resource_deferred(ID3D12Resource* resource, uint64_t frame);

    [[nodiscard]] const Buffer& get_buffer(Buffer_Handle handle) const;
    [[nodiscard]] Buffer& get_buffer(Buffer_Handle handle);
    [[nodiscard]] const Texture& get_texture(Texture_Handle handle) const;
    [[nodiscard]] Texture& get_texture(Texture_Handle handle);
    [[nodiscard]] const Pipeline& get_pipeline(Pipeline_Handle handle) const;
    [[nodiscard]] Pipeline& get_pipeline(Pipeline_Handle handle);
    [[nodiscard]] const Shader& get_shader(Shader_Handle handle) const;
    [[nodiscard]] Shader& get_shader(Shader_Handle handle);

    void empty_deletion_queues(uint64_t frame);

private:
    D3D12_Context* m_ctx;

    Resource_Allocator<Buffer> m_buffers;
    Resource_Allocator<Texture> m_textures;
    Resource_Allocator<Pipeline> m_pipelines;
    Resource_Allocator<Shader> m_shaders;
    Resource_Allocator<Sampler> m_samplers;

    Descriptor_Allocator m_cbv_srv_uav_descriptor_allocator;
    Descriptor_Allocator m_sampler_descriptor_allocator;
    Descriptor_Allocator m_rtv_descriptor_allocator;
    Descriptor_Allocator m_dsv_descriptor_allocator;

    template<typename T>
    struct Deletion_Queue_Resource
    {
        T resource;
        uint64_t frame;
    };
    std::vector<Deletion_Queue_Resource<Buffer_Handle>> m_buffer_deletion_queue;
    std::vector<Deletion_Queue_Resource<Texture_Handle>> m_texture_deletion_queue;
    std::vector<Deletion_Queue_Resource<Pipeline_Handle>> m_pipeline_deletion_queue;
    std::vector<Deletion_Queue_Resource<Sampler_Handle>> m_sampler_deletion_queue;
    std::vector<Deletion_Queue_Resource<ID3D12Resource*>> m_deferred_resource_deletion_queue;

    Com_Ptr<IDxcUtils> m_dxc_utils;
};
}
