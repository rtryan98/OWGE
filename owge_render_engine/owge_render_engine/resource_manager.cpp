#include "owge_render_engine/resource_manager.hpp"

#include "owge_common/file_util.hpp"
#include "owge_d3d12_base/d3d12_ctx.hpp"

#include <algorithm>
#include <d3d12shader.h>
#include <ranges>

namespace owge
{
static constexpr uint32_t MAX_BUFFERS = 1048576;
static constexpr uint32_t MAX_TEXTURES = 1048576;
static constexpr uint32_t MAX_PIPELINES = 262144;
static constexpr uint32_t MAX_SHADERS = 524288;

static constexpr uint32_t NO_SRV = 0x1FFFFF;
static constexpr uint32_t NO_UAV = 0x1FFFFF;
static constexpr uint32_t NO_RTV_DSV = 0x1FFFFF;

Resource_Manager::Resource_Manager(D3D12_Context* ctx)
    : m_ctx(ctx)
    , m_buffers(MAX_BUFFERS)
    , m_textures(MAX_TEXTURES)
    , m_pipelines(MAX_PIPELINES)
    , m_shaders(MAX_SHADERS)
    , m_samplers(D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE)
    , m_cbv_srv_uav_descriptor_allocator(m_ctx->cbv_srv_uav_descriptor_heap, m_ctx->device)
    , m_sampler_descriptor_allocator(m_ctx->sampler_descriptor_heap, m_ctx->device)
    , m_rtv_descriptor_allocator(m_ctx->rtv_descriptor_heap, m_ctx->device)
    , m_dsv_descriptor_allocator(m_ctx->dsv_descriptor_heap, m_ctx->device)
{
    throw_if_failed(DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&m_dxc_utils)),
        "Failed to create DxcUtils.");
}

Buffer_Handle Resource_Manager::create_buffer(const Buffer_Desc & desc, const wchar_t* name)
{
    Buffer buffer = {};

    D3D12_RESOURCE_DESC1 resource_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = desc.size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = {.Count = 1, .Quality = 0 },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
        .SamplerFeedbackMipRegion = {}
    };
    D3D12_HEAP_PROPERTIES heap_properties = {
        .Type = desc.heap_type,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0
    };
    if (desc.usage == Resource_Usage::Read_Write)
    {
        resource_desc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    }
    m_ctx->device->CreateCommittedResource3(
        &heap_properties, D3D12_HEAP_FLAG_NONE,
        &resource_desc, D3D12_BARRIER_LAYOUT_UNDEFINED, nullptr,
        nullptr, 0, nullptr, IID_PPV_ARGS(&buffer.resource));
    if (name)
    {
        buffer.resource->SetName(name);
    }

    auto srv = m_cbv_srv_uav_descriptor_allocator.allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
        .Format = DXGI_FORMAT_R32_TYPELESS,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer = {
            .FirstElement = 0,
            .NumElements = uint32_t(desc.size) >> 2,
            .StructureByteStride = 0,
            .Flags = D3D12_BUFFER_SRV_FLAG_RAW
        }
    };
    m_ctx->device->CreateShaderResourceView(buffer.resource, &srv_desc, srv.cpu_handle);

    auto uav = m_cbv_srv_uav_descriptor_allocator.allocate();
    if (desc.usage == Resource_Usage::Read_Write)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
            .Format = DXGI_FORMAT_R32_TYPELESS,
            .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = uint32_t(desc.size) >> 2,
                .StructureByteStride = 0,
                .CounterOffsetInBytes = 0,
                .Flags = D3D12_BUFFER_UAV_FLAG_RAW
            }
        };
        m_ctx->device->CreateUnorderedAccessView(buffer.resource, nullptr, &uav_desc, uav.cpu_handle);
    }

    return m_buffers.insert(0, srv.index, buffer);
}

Texture_Handle Resource_Manager::create_texture(const Texture_Desc& desc, const wchar_t* name)
{
    Texture texture = {};

    D3D12_RESOURCE_DESC1 resource_desc = {
        .Dimension = desc.dimension,
        .Alignment = 0,
        .Width = desc.width,
        .Height = desc.height,
        .DepthOrArraySize = uint16_t(desc.depth_or_array_layers),
        .MipLevels = uint16_t(desc.mip_levels),
        .Format = desc.format,
        .SampleDesc = {.Count = 1, .Quality = 1 },
        .Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
        .SamplerFeedbackMipRegion = {}
    };
    D3D12_HEAP_PROPERTIES heap_properties = {
        .Type = D3D12_HEAP_TYPE_DEFAULT,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0
    };
    resource_desc.Flags |= desc.uav_dimension != D3D12_UAV_DIMENSION_UNKNOWN
        ? D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS
        : D3D12_RESOURCE_FLAG_NONE;
    resource_desc.Flags |= desc.rtv_dimension != D3D12_RTV_DIMENSION_UNKNOWN
        ? D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET
        : D3D12_RESOURCE_FLAG_NONE;
    resource_desc.Flags |= desc.dsv_dimension != D3D12_DSV_DIMENSION_UNKNOWN
        ? D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
        : D3D12_RESOURCE_FLAG_NONE;
    m_ctx->device->CreateCommittedResource3(
        &heap_properties, D3D12_HEAP_FLAG_NONE,
        &resource_desc, desc.initial_layout, nullptr,
        nullptr, 0, nullptr, IID_PPV_ARGS(&texture.resource));
    if (name)
    {
        texture.resource->SetName(name);
    }

    auto srv = m_cbv_srv_uav_descriptor_allocator.allocate();
    if (desc.srv_dimension != D3D12_SRV_DIMENSION_UNKNOWN)
    {
        D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
            .Format = desc.format,
            .ViewDimension = desc.srv_dimension,
            .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        };
        switch (desc.srv_dimension)
        {
        case D3D12_SRV_DIMENSION_UNKNOWN:
            // TODO: insert warning, assert or similar.
            break;
        case D3D12_SRV_DIMENSION_TEXTURE1D:
            srv_desc.Texture1D = {
                .MostDetailedMip = 0,
                .MipLevels = ~0u,
                .ResourceMinLODClamp = 0.0f
            };
            break;
        case D3D12_SRV_DIMENSION_TEXTURE1DARRAY:
            srv_desc.Texture1DArray = {
                .MostDetailedMip = 0,
                .MipLevels = ~0u,
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers,
                .ResourceMinLODClamp = 0.0f
            };
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2D:
            srv_desc.Texture2D = {
                .MostDetailedMip = 0,
                .MipLevels = ~0u,
                .PlaneSlice = 0,
                .ResourceMinLODClamp = 0.0f
            };
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2DARRAY:
            srv_desc.Texture2DArray = {
                .MostDetailedMip = 0,
                .MipLevels = ~0u,
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers,
                .PlaneSlice = 0,
                .ResourceMinLODClamp = 0.0f
            };
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2DMS:
            srv_desc.Texture2DMS = {
                .UnusedField_NothingToDefine = 0
            };
            break;
        case D3D12_SRV_DIMENSION_TEXTURE2DMSARRAY:
            srv_desc.Texture2DMSArray = {
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers
            };
            break;
        case D3D12_SRV_DIMENSION_TEXTURE3D:
            srv_desc.Texture3D = {
                .MostDetailedMip = 0,
                .MipLevels = ~0u,
                .ResourceMinLODClamp = 0.0f
            };
            break;
        case D3D12_SRV_DIMENSION_TEXTURECUBE:
            srv_desc.TextureCube = {
                .MostDetailedMip = 0,
                .MipLevels = ~0u,
                .ResourceMinLODClamp = 0.0f
            };
            break;
        case D3D12_SRV_DIMENSION_TEXTURECUBEARRAY:
            srv_desc.TextureCubeArray = {
                .MostDetailedMip = 0,
                .MipLevels = ~0u,
                .First2DArrayFace = 0,
                .NumCubes = desc.depth_or_array_layers / 6,
                .ResourceMinLODClamp = 0.0f
            };
            break;
        case D3D12_SRV_DIMENSION_BUFFER:
            [[fallthrough]];
        case D3D12_SRV_DIMENSION_RAYTRACING_ACCELERATION_STRUCTURE:
            [[fallthrough]];
        default:
            std::unreachable();
            break;
        }
        m_ctx->device->CreateShaderResourceView(texture.resource, &srv_desc, srv.cpu_handle);
    }

    auto uav = m_cbv_srv_uav_descriptor_allocator.allocate();
    if (desc.uav_dimension != D3D12_UAV_DIMENSION_UNKNOWN)
    {
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
            .Format = desc.format,
            .ViewDimension = desc.uav_dimension,
        };
        switch (desc.uav_dimension)
        {
        case D3D12_UAV_DIMENSION_TEXTURE1D:
            uav_desc.Texture1D = {
                .MipSlice = 0
            };
            break;
        case D3D12_UAV_DIMENSION_TEXTURE1DARRAY:
            uav_desc.Texture1DArray = {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers
            };
            break;
        case D3D12_UAV_DIMENSION_TEXTURE2D:
            uav_desc.Texture2D = {
                .MipSlice = 0
            };
            break;
        case D3D12_UAV_DIMENSION_TEXTURE2DARRAY:
            uav_desc.Texture2DArray = {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers,
                .PlaneSlice = 0
            };
            break;
        case D3D12_UAV_DIMENSION_TEXTURE2DMS:
            uav_desc.Texture2DMS = {
                .UnusedField_NothingToDefine = 0
            };
            break;
        case D3D12_UAV_DIMENSION_TEXTURE2DMSARRAY:
            uav_desc.Texture2DMSArray = {
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers
            };
            break;
        case D3D12_UAV_DIMENSION_TEXTURE3D:
            uav_desc.Texture3D = {
                .MipSlice = 0,
                .FirstWSlice = 0,
                .WSize = ~0u
            };
            break;
        case D3D12_UAV_DIMENSION_BUFFER:
            [[fallthrough]];
        default:
            std::unreachable();
            break;
        }
        m_ctx->device->CreateUnorderedAccessView(texture.resource, nullptr, &uav_desc, uav.cpu_handle);
    }

    texture.rtv = NO_RTV_DSV;
    texture.dsv = NO_RTV_DSV;
    // TODO: check if RTV and DSV dimensions are set?
    if (desc.rtv_dimension != D3D12_RTV_DIMENSION_UNKNOWN)
    {
        auto rtv = m_rtv_descriptor_allocator.allocate();
        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
            .Format = desc.format,
            .ViewDimension = desc.rtv_dimension
        };
        switch (desc.rtv_dimension)
        {
        case D3D12_RTV_DIMENSION_TEXTURE1D:
            rtv_desc.Texture1D = {
                .MipSlice = 0
            };
            break;
        case D3D12_RTV_DIMENSION_TEXTURE1DARRAY:
            rtv_desc.Texture1DArray = {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers
            };
            break;
        case D3D12_RTV_DIMENSION_TEXTURE2D:
            rtv_desc.Texture2D = {
                .MipSlice = 0,
                .PlaneSlice = 0
            };
            break;
        case D3D12_RTV_DIMENSION_TEXTURE2DARRAY:
            rtv_desc.Texture2DArray = {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers,
                .PlaneSlice = 0
            };
            break;
        case D3D12_RTV_DIMENSION_TEXTURE2DMS:
            rtv_desc.Texture2DMS = {
                .UnusedField_NothingToDefine = 0
            };
            break;
        case D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY:
            rtv_desc.Texture2DMSArray = {
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers
            };
            break;
        default:
            std::unreachable();
            break;
        }
        m_ctx->device->CreateRenderTargetView(texture.resource, &rtv_desc, rtv.cpu_handle);
        texture.rtv = rtv.index;
    }
    else if (desc.dsv_dimension != D3D12_DSV_DIMENSION_UNKNOWN)
    {
        auto dsv = m_dsv_descriptor_allocator.allocate();
        D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc = {
            .Format = desc.format,
            .ViewDimension = desc.dsv_dimension
        };
        switch (desc.dsv_dimension)
        {
        case D3D12_DSV_DIMENSION_TEXTURE1D:
            dsv_desc.Texture1D = {
                .MipSlice = 0
            };
            break;
        case D3D12_DSV_DIMENSION_TEXTURE1DARRAY:
            dsv_desc.Texture1DArray = {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers
            };
            break;
        case D3D12_DSV_DIMENSION_TEXTURE2D:
            dsv_desc.Texture2D = {
                .MipSlice = 0
            };
            break;
        case D3D12_DSV_DIMENSION_TEXTURE2DARRAY:
            dsv_desc.Texture2DArray = {
                .MipSlice = 0,
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers
            };
            break;
        case D3D12_DSV_DIMENSION_TEXTURE2DMS:
            dsv_desc.Texture2DMS = {
                .UnusedField_NothingToDefine = 0
            };
            break;
        case D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY:
            dsv_desc.Texture2DMSArray = {
                .FirstArraySlice = 0,
                .ArraySize = desc.depth_or_array_layers
            };
            break;
        default:
            std::unreachable();
            break;
        }
        m_ctx->device->CreateDepthStencilView(texture.resource, &dsv_desc, dsv.cpu_handle);
        texture.dsv = dsv.index;
    }

    return m_textures.insert(0, srv.index, texture);
}

Shader_Handle Resource_Manager::create_shader(const Shader_Desc& desc)
{
    auto emplaced_handle = m_shaders.emplace(0, 0);
    emplaced_handle.value.path = desc.path;
    emplaced_handle.value.bytecode = read_file_as_binary(desc.path.c_str());
    return emplaced_handle.handle;
}

Pipeline_Handle Resource_Manager::create_pipeline(const Graphics_Pipeline_Desc& desc, const wchar_t* name)
{
    Pipeline pipeline = {
        .type = Pipeline_Type::Graphics
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {
        .pRootSignature = m_ctx->global_rootsig,
        .StreamOutput = {},
        .BlendState = desc.blend_state,
        .SampleMask = ~0u,
        .RasterizerState = desc.rasterizer_state,
        .DepthStencilState = desc.depth_stencil_state,
        .InputLayout = {},
        .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        .PrimitiveTopologyType = desc.primitive_topology_type,
        .NumRenderTargets = desc.rtv_count,
        .RTVFormats = {}, // memcpy
        .DSVFormat = desc.dsv_format,
        .SampleDesc = {.Count = 1, .Quality = 0 },
        .NodeMask = 0,
        .CachedPSO = {},
        .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
    };
    memcpy(&pso_desc.RTVFormats, &desc.rtv_formats, sizeof(DXGI_FORMAT) * 8);
    if (!desc.shaders.vs.is_null_handle())
    {
        auto& shader = get_shader(desc.shaders.vs);
        pso_desc.VS = {
            .pShaderBytecode = shader.bytecode.data(),
            .BytecodeLength = shader.bytecode.size()
        };
    }
    if (!desc.shaders.ps.is_null_handle())
    {
        auto& shader = get_shader(desc.shaders.ps);
        pso_desc.PS = {
            .pShaderBytecode = shader.bytecode.data(),
            .BytecodeLength = shader.bytecode.size()
        };
    }
    if (!desc.shaders.ds.is_null_handle())
    {
        auto& shader = get_shader(desc.shaders.ds);
        pso_desc.DS = {
            .pShaderBytecode = shader.bytecode.data(),
            .BytecodeLength = shader.bytecode.size()
        };
    }
    if (!desc.shaders.hs.is_null_handle())
    {
        auto& shader = get_shader(desc.shaders.hs);
        pso_desc.HS = {
            .pShaderBytecode = shader.bytecode.data(),
            .BytecodeLength = shader.bytecode.size()
        };
    }
    if (!desc.shaders.gs.is_null_handle())
    {
        auto& shader = get_shader(desc.shaders.gs);
        pso_desc.GS = {
            .pShaderBytecode = shader.bytecode.data(),
            .BytecodeLength = shader.bytecode.size()
        };
    }
    m_ctx->device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline.pso));
    if (name)
    {
        pipeline.pso->SetName(name);
    }

    return m_pipelines.insert(0, 0, pipeline);
}

Pipeline_Handle Resource_Manager::create_pipeline(const Compute_Pipeline_Desc& desc, const wchar_t* name)
{
    Pipeline pipeline = {
        .type = Pipeline_Type::Compute
    };

    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {
        .pRootSignature = m_ctx->global_rootsig,
        .NodeMask = 0,
        .CachedPSO = {},
        .Flags = D3D12_PIPELINE_STATE_FLAG_NONE
    };
    if (!desc.cs.is_null_handle())
    {
        auto& shader = get_shader(desc.cs);
        pso_desc.CS = {
            .pShaderBytecode = shader.bytecode.data(),
            .BytecodeLength = shader.bytecode.size()
        };

        Com_Ptr<ID3D12ShaderReflection> shader_reflection = {};
        DxcBuffer reflection_buffer = {
            .Ptr = shader.bytecode.data(),
            .Size = shader.bytecode.size(),
            .Encoding = DXC_CP_ACP,
        };
        m_dxc_utils->CreateReflection(&reflection_buffer, IID_PPV_ARGS(&shader_reflection));
        shader_reflection->GetThreadGroupSize(
            &pipeline.workgroups_x, &pipeline.workgroups_y, &pipeline.workgroups_z);
    }
    m_ctx->device->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&pipeline.pso));
    if (name)
    {
        pipeline.pso->SetName(name);
    }

    return m_pipelines.insert(0, 0, pipeline);
}

Sampler_Handle Resource_Manager::create_sampler(const Sampler_Desc& desc)
{
    D3D12_SAMPLER_DESC sampler_desc = {};
    memcpy(&sampler_desc, &desc, sizeof(D3D12_SAMPLER_DESC));
    auto descriptor = m_sampler_descriptor_allocator.allocate();
    m_ctx->device->CreateSampler(&sampler_desc, descriptor.cpu_handle);
    return m_samplers.insert(0, descriptor.index, {});
}

void Resource_Manager::destroy_buffer(Buffer_Handle handle, uint64_t frame)
{
    m_buffer_deletion_queue.push_back({ handle, frame });
}

void Resource_Manager::destroy_texture(Texture_Handle handle, uint64_t frame)
{
    m_texture_deletion_queue.push_back({ handle, frame });
}

void Resource_Manager::destroy_shader(Shader_Handle handle)
{
    m_shaders.remove(handle);
}

void Resource_Manager::destroy_pipeline(Pipeline_Handle handle, uint64_t frame)
{
    m_pipeline_deletion_queue.push_back({ handle, frame });
}

void Resource_Manager::destroy_sampler(Sampler_Handle handle, uint64_t frame)
{
    m_sampler_deletion_queue.push_back({ handle, frame });
}

void Resource_Manager::destroy_d3d12_resource_deferred(ID3D12Resource* resource, uint64_t frame)
{
    m_deferred_resource_deletion_queue.push_back({ resource, frame });
}

const Buffer& Resource_Manager::get_buffer(Buffer_Handle handle) const
{
    return m_buffers.at(handle);
}

Buffer& Resource_Manager::get_buffer(Buffer_Handle handle)
{
    return m_buffers[handle];
}

const Texture& Resource_Manager::get_texture(Texture_Handle handle) const
{
    return m_textures.at(handle);
}

Texture& Resource_Manager::get_texture(Texture_Handle handle)
{
    return m_textures[handle];
}

const Pipeline& Resource_Manager::get_pipeline(Pipeline_Handle handle) const
{
    return m_pipelines.at(handle);
}

Pipeline& Resource_Manager::get_pipeline(Pipeline_Handle handle)
{
    return m_pipelines[handle];
}

const Shader& Resource_Manager::get_shader(Shader_Handle handle) const
{
    return m_shaders.at(handle);
}

Shader& Resource_Manager::get_shader(Shader_Handle handle)
{
    return m_shaders[handle];
}

void Resource_Manager::empty_deletion_queues(uint64_t frame)
{
    auto buffer_range = std::ranges::remove_if(m_buffer_deletion_queue, [this, frame](auto& element) {
        if (frame >= element.frame)
        {
            auto& buffer = m_buffers[element.resource];
            buffer.resource->Release();
            m_cbv_srv_uav_descriptor_allocator.free(uint32_t(element.resource.bindless_idx + 1));
            m_cbv_srv_uav_descriptor_allocator.free(element.resource.bindless_idx);
            m_buffers.remove(element.resource);
            return true;
        }
        return false;
        });
    m_buffer_deletion_queue.erase(buffer_range.begin(), buffer_range.end());

    auto texture_range = std::ranges::remove_if(m_texture_deletion_queue, [this, frame](auto& element) {
        if (frame >= element.frame)
        {
            auto& texture = m_textures[element.resource];
            texture.resource->Release();
            m_cbv_srv_uav_descriptor_allocator.free(uint32_t(element.resource.bindless_idx + 1));
            m_cbv_srv_uav_descriptor_allocator.free(element.resource.bindless_idx);
            m_textures.remove(element.resource);
            return true;
        }
        return false;
        });
    m_texture_deletion_queue.erase(texture_range.begin(), texture_range.end());

    auto pipeline_range = std::ranges::remove_if(m_pipeline_deletion_queue, [this, frame](auto& element) {
        if (frame >= element.frame)
        {
            auto& pso = m_pipelines[element.resource];
            pso.pso->Release();
            m_pipelines.remove(element.resource);
            return true;
        }
        return false;
        });
    m_pipeline_deletion_queue.erase(pipeline_range.begin(), pipeline_range.end());

    auto sampler_range = std::ranges::remove_if(m_sampler_deletion_queue, [this, frame](auto& element) {
        if (frame >= element.frame)
        {
            m_sampler_descriptor_allocator.free(element.resource.bindless_idx);
            return true;
        }
        return false;
        });
    m_sampler_deletion_queue.erase(sampler_range.begin(), sampler_range.end());

    auto resource_range = std::ranges::remove_if(m_deferred_resource_deletion_queue, [this, frame](auto& element) {
        if (frame >= element.frame)
        {
            element.resource->Release();
            return true;
        }
        return false;
        });
    m_deferred_resource_deletion_queue.erase(resource_range.begin(), resource_range.end());
}
}
