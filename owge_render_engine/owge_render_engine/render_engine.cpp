#include "owge_render_engine/render_engine.hpp"

#include <algorithm>
#include <utility>
#include <fstream>
#include <ranges>

#if OWGE_USE_NVPERF
#pragma warning(push, 3) // NvPerf sample code does not compile under W4
#include <nvperf_host_impl.h>
#include <NvPerfD3D12.h>
#pragma warning(pop)
#endif

namespace owge
{
static constexpr uint32_t MAX_BUFFERS = 1048576;
static constexpr uint32_t MAX_TEXTURES = 1048576;
static constexpr uint32_t MAX_PIPELINES = 262144;
static constexpr uint32_t MAX_SHADERS = 524288;

static constexpr uint32_t NO_SRV = ~0u;
static constexpr uint32_t NO_UAV = ~0u;
static constexpr uint32_t NO_RTV_DSV = ~0u;

Render_Engine::Render_Engine(HWND hwnd,
    const D3D12_Context_Settings& d3d12_context_settings,
    const Render_Engine_Settings& render_engine_settings)
    : m_ctx(),
    m_settings(render_engine_settings),
    m_swapchain(),
    m_buffers(MAX_BUFFERS),
    m_textures(MAX_TEXTURES),
    m_pipelines(MAX_PIPELINES),
    m_shaders(MAX_SHADERS)
{
    m_ctx = create_d3d12_context(&d3d12_context_settings);
    m_swapchain = std::make_unique<D3D12_Swapchain>(
        m_ctx.factory, m_ctx.device, m_ctx.direct_queue,
        hwnd, MAX_SWAPCHAIN_BUFFERS);
    D3D12_DESCRIPTOR_HEAP_DESC rtv_dsv_descriptor_heap_desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = MAX_RTV_DSV_DESCRIPTORS,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0
    };
    m_ctx.device->CreateDescriptorHeap(&rtv_dsv_descriptor_heap_desc, IID_PPV_ARGS(&m_rtv_descriptor_heap));
    rtv_dsv_descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    m_ctx.device->CreateDescriptorHeap(&rtv_dsv_descriptor_heap_desc, IID_PPV_ARGS(&m_dsv_descriptor_heap));

    m_cbv_srv_uav_descriptor_allocator = std::make_unique<Descriptor_Allocator>(
        m_ctx.cbv_srv_uav_descriptor_heap, m_ctx.device);
    m_sampler_descriptor_allocator = std::make_unique<Descriptor_Allocator>(
        m_ctx.sampler_descriptor_heap, m_ctx.device);
    m_rtv_descriptor_allocator = std::make_unique<Descriptor_Allocator>(
        m_rtv_descriptor_heap.Get(), m_ctx.device);
    m_dsv_descriptor_allocator = std::make_unique<Descriptor_Allocator>(
        m_dsv_descriptor_heap.Get(), m_ctx.device);

    for (auto i = 0; i < MAX_CONCURRENT_GPU_FRAMES; ++i)
    {
        auto& frame_ctx = m_frame_contexts[i];
        frame_ctx.direct_queue_cmd_alloc = std::make_unique<Command_Allocator>(
            m_ctx.device, D3D12_COMMAND_LIST_TYPE_DIRECT);
        m_ctx.device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&frame_ctx.direct_queue_fence));
        frame_ctx.frame_number = 0;
        frame_ctx.staging_buffer_allocator = std::make_unique<Staging_Buffer_Allocator>(
            m_ctx.device);
    }

    m_bindset_stager = std::make_unique<Bindset_Stager>();

    if (render_engine_settings.nvperf_enabled)
    {
#if OWGE_USE_NVPERF
        if (d3d12_context_settings.enable_validation ||
            d3d12_context_settings.enable_gpu_based_validation)
        {
            // TODO: nvperf does not support validation. Post warning.
            printf("nvperf cannot be enabled at the same time as validation layers.\n");
            return;
        }
        if (!nv::perf::InitializeNvPerf())
        {
            // TODO: log failed init.
            printf("nvperf failed to initialize.\n");
            return;
        }
        if (!nv::perf::D3D12LoadDriver())
        {
            // TODO: log no driver
            printf("nvperf driver load failed.\n");
            return;
        }
        if (!nv::perf::D3D12IsNvidiaDevice(m_ctx.device))
        {
            // TODO: log no nv device
            printf("nvperf enabled but device is not a nvidia device.\n");
            return;
        }
        if (!nv::perf::profiler::D3D12IsGpuSupported(m_ctx.device))
        {
            // TODO: log unsupported nv device
            printf("nvperf enabled but gpu is unsupported.\n");
            return;
        }
        if (render_engine_settings.nvperf_lock_clocks_to_rated_tdp)
        {
            nv::perf::D3D12SetDeviceClockState(m_ctx.device, NVPW_DEVICE_CLOCK_SETTING_LOCK_TO_RATED_TDP);
        }
        // TODO: implement nvperf
        m_nvperf_active = true;
        printf("nvperf was loaded successfully.\n");
#endif
    }
}

Render_Engine::~Render_Engine()
{
    d3d12_context_wait_idle(&m_ctx);
    empty_deletion_queues(~0ull);

#if OWGE_USE_NVPERF
    if (m_nvperf_active &&
        m_settings.nvperf_lock_clocks_to_rated_tdp)
    {
        nv::perf::D3D12SetDeviceClockState(m_ctx.device, NVPW_DEVICE_CLOCK_SETTING_DEFAULT);
    }
#endif

    m_swapchain = nullptr;
    destroy_d3d12_context(&m_ctx);
}

void Render_Engine::add_procedure(Render_Procedure* proc)
{
    m_procedures.push_back(proc);
}

void Render_Engine::render()
{
    auto& frame_ctx = m_frame_contexts[m_current_frame_index];

    if (wait_for_d3d12_fence(frame_ctx.direct_queue_fence.Get(), frame_ctx.frame_number, INFINITE) != WAIT_OBJECT_0)
    {
        // TODO: wait error. Warn about possible desync?
    }
    frame_ctx.direct_queue_cmd_alloc->reset();
    frame_ctx.staging_buffer_allocator->reset();
    empty_deletion_queues(m_current_frame);

    if (m_swapchain->try_resize())
    {
        // TODO: client window area dependent resources
    }
    m_swapchain->acquire_next_image();

    auto procedure_cmd = frame_ctx.direct_queue_cmd_alloc->get_or_allocate();
    frame_ctx.upload_cmd = frame_ctx.direct_queue_cmd_alloc->get_or_allocate().cmd;

    Render_Procedure_Payload proc_payload = {
        .render_engine = this,
        .cmd = procedure_cmd.cmd,
        .barrier_builder = nullptr,
        .swapchain = m_swapchain.get()
    };
    procedure_cmd.cmd->SetComputeRootSignature(m_ctx.global_rootsig);
    procedure_cmd.cmd->SetGraphicsRootSignature(m_ctx.global_rootsig);
    auto descriptor_heaps = std::to_array({
        m_ctx.cbv_srv_uav_descriptor_heap, m_ctx.sampler_descriptor_heap
        });
    procedure_cmd.cmd->SetDescriptorHeaps(uint32_t(descriptor_heaps.size()), descriptor_heaps.data());
    for (auto procedure : m_procedures)
    {
        procedure->process(proc_payload);
    }

    m_bindset_stager->process(procedure_cmd.cmd);

    frame_ctx.upload_cmd->Close();
    procedure_cmd.cmd->Close();
    auto cmds = std::to_array({
        static_cast<ID3D12CommandList*>(frame_ctx.upload_cmd),
        static_cast<ID3D12CommandList*>(procedure_cmd.cmd) });
    m_ctx.direct_queue->ExecuteCommandLists(uint32_t(cmds.size()), cmds.data());

    auto swapchain = m_swapchain->get_swapchain();
    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
    swapchain->GetDesc1(&swapchain_desc);
    auto allow_tearing = swapchain_desc.Flags & DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING
        ? DXGI_PRESENT_ALLOW_TEARING
        : 0u;
    swapchain->Present(0, allow_tearing);

    m_current_frame += 1;
    m_current_frame_index = m_current_frame % MAX_CONCURRENT_GPU_FRAMES;
    frame_ctx.frame_number += 1;

    m_ctx.direct_queue->Signal(frame_ctx.direct_queue_fence.Get(), frame_ctx.frame_number);
}

void Render_Engine::update_bindings(const Bindset& bindset)
{
    auto& frame_ctx = m_frame_contexts[m_current_frame_index];
    m_bindset_stager->stage_bindset(this, bindset, frame_ctx.staging_buffer_allocator.get());
}

Buffer_Handle Render_Engine::create_buffer(const Buffer_Desc& desc)
{
    Buffer buffer = {};

    D3D12_RESOURCE_DESC1 resource_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = desc.size,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_R8_UNORM,
        .SampleDesc = { .Count = 1, .Quality = 0 },
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
    m_ctx.device->CreateCommittedResource3(
        &heap_properties, D3D12_HEAP_FLAG_NONE,
        &resource_desc, desc.initial_layout, nullptr,
        nullptr, 0, nullptr, IID_PPV_ARGS(&buffer.resource));

    buffer.srv = NO_SRV;
    auto srv = m_cbv_srv_uav_descriptor_allocator->allocate();
    D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
        .Format = DXGI_FORMAT_R32_TYPELESS,
        .ViewDimension = D3D12_SRV_DIMENSION_BUFFER,
        .Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
        .Buffer = {
            .FirstElement = 0,
            .NumElements = 0,
            .StructureByteStride = 0,
            .Flags = D3D12_BUFFER_SRV_FLAG_RAW
        }
    };
    buffer.srv = srv.index;
    m_ctx.device->CreateShaderResourceView(buffer.resource, &srv_desc, srv.cpu_handle);

    buffer.uav = NO_UAV;
    if (desc.usage == Resource_Usage::Read_Write)
    {
        auto uav = m_cbv_srv_uav_descriptor_allocator->allocate();
        D3D12_UNORDERED_ACCESS_VIEW_DESC uav_desc = {
            .Format = DXGI_FORMAT_R32_TYPELESS,
            .ViewDimension = D3D12_UAV_DIMENSION_BUFFER,
            .Buffer = {
                .FirstElement = 0,
                .NumElements = 0,
                .StructureByteStride = 0,
                .CounterOffsetInBytes = 0,
                .Flags = D3D12_BUFFER_UAV_FLAG_RAW
            }
        };
        m_ctx.device->CreateUnorderedAccessView(buffer.resource, nullptr, &uav_desc, uav.cpu_handle);
        buffer.uav = uav.index;
    }

    return m_buffers.insert(0, buffer);
}

Texture_Handle Render_Engine::create_texture(const Texture_Desc& desc)
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
    m_ctx.device->CreateCommittedResource3(
        &heap_properties, D3D12_HEAP_FLAG_NONE,
        &resource_desc, desc.initial_layout, nullptr,
        nullptr, 0, nullptr, IID_PPV_ARGS(&texture.resource));

    texture.srv = NO_SRV;
    if (desc.srv_dimension != D3D12_SRV_DIMENSION_UNKNOWN)
    {
        auto srv = m_cbv_srv_uav_descriptor_allocator->allocate();
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
        m_ctx.device->CreateShaderResourceView(texture.resource, &srv_desc, srv.cpu_handle);
        texture.srv = srv.index;
    }

    texture.uav = NO_UAV;
    if (desc.uav_dimension != D3D12_UAV_DIMENSION_UNKNOWN)
    {
        auto uav = m_cbv_srv_uav_descriptor_allocator->allocate();
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
                .MipSlice = 0
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
        m_ctx.device->CreateUnorderedAccessView(texture.resource, nullptr, &uav_desc, uav.cpu_handle);
        texture.uav = uav.index;
    }

    texture.rtv_dsv = NO_RTV_DSV;
    // TODO: check if RTV and DSV dimensions are set?
    if (desc.rtv_dimension != D3D12_RTV_DIMENSION_UNKNOWN)
    {
        auto rtv = m_rtv_descriptor_allocator->allocate();
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
        m_ctx.device->CreateRenderTargetView(texture.resource, &rtv_desc, rtv.cpu_handle);
        texture.rtv_dsv = rtv.index;
    }
    else if (desc.dsv_dimension != D3D12_DSV_DIMENSION_UNKNOWN)
    {
        auto dsv = m_dsv_descriptor_allocator->allocate();
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
        m_ctx.device->CreateDepthStencilView(texture.resource, &dsv_desc, dsv.cpu_handle);
        texture.rtv_dsv = dsv.index;
    }

    return m_textures.insert(0, texture);
}

Shader_Handle Render_Engine::create_shader(const Shader_Desc& desc)
{
    auto emplaced_handle = m_shaders.emplace(0);
    emplaced_handle.value.path = desc.path;
    emplaced_handle.value.bytecode = read_shader_from_file(desc.path);
    return emplaced_handle.handle;
}

Pipeline_Handle Render_Engine::create_pipeline(const Graphics_Pipeline_Desc& desc)
{
    Pipeline pipeline = {
        .type = Pipeline_Type::Graphics
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {
        .pRootSignature = m_ctx.global_rootsig,
        .StreamOutput = {},
        .BlendState = desc.blend_state,
        .SampleMask = ~0u,
        .RasterizerState = desc.rasterizer_state,
        .DepthStencilState = desc.depth_stencil_state,
        .InputLayout = {},
        .IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED,
        .PrimitiveTopologyType = desc.primitive_topology_type,
        .NumRenderTargets = 0,
        .RTVFormats = {},
        .DSVFormat = desc.dsv_format,
        .SampleDesc = { .Count = 1, .Quality = 0 },
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
    m_ctx.device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&pipeline.pso));

    return m_pipelines.insert(0, pipeline);
}

Pipeline_Handle Render_Engine::create_pipeline(const Compute_Pipeline_Desc& desc)
{
    Pipeline pipeline = {
        .type = Pipeline_Type::Compute
    };

    D3D12_COMPUTE_PIPELINE_STATE_DESC pso_desc = {
        .pRootSignature = m_ctx.global_rootsig,
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
    }
    m_ctx.device->CreateComputePipelineState(&pso_desc, IID_PPV_ARGS(&pipeline.pso));

    return m_pipelines.insert(0, pipeline);
}

void Render_Engine::destroy_buffer(Buffer_Handle handle)
{
    m_buffer_deletion_queue.push_back({ handle, m_current_frame + MAX_CONCURRENT_GPU_FRAMES });
}

void Render_Engine::destroy_texture(Texture_Handle handle)
{
    m_texture_deletion_queue.push_back({ handle, m_current_frame + MAX_CONCURRENT_GPU_FRAMES });
}

void Render_Engine::destroy_shader(Shader_Handle handle)
{
    m_shaders.remove(handle);
}

void Render_Engine::destroy_pipeline(Pipeline_Handle handle)
{
    m_pipeline_deletion_queue.push_back({ handle, m_current_frame + MAX_CONCURRENT_GPU_FRAMES });
}

const Buffer& Render_Engine::get_buffer(Buffer_Handle handle) const
{
    return m_buffers.at(handle);
}

Buffer& Render_Engine::get_buffer(Buffer_Handle handle)
{
    return m_buffers[handle];
}

const Texture& Render_Engine::get_texture(Texture_Handle handle) const
{
    return m_textures.at(handle);
}

Texture& Render_Engine::get_texture(Texture_Handle handle)
{
    return m_textures[handle];
}

const Pipeline& Render_Engine::get_pipeline(Pipeline_Handle handle) const
{
    return m_pipelines.at(handle);
}

Pipeline& Render_Engine::get_pipeline(Pipeline_Handle handle)
{
    return m_pipelines[handle];
}

const Shader& Render_Engine::get_shader(Shader_Handle handle) const
{
    return m_shaders.at(handle);
}

Shader& Render_Engine::get_shader(Shader_Handle handle)
{
    return m_shaders[handle];
}

std::vector<uint8_t> Render_Engine::read_shader_from_file(const std::string& path)
{
    std::vector<uint8_t> result;
    std::ifstream file(path, std::ios::binary);
    file.unsetf(std::ios::skipws);
    std::streampos file_size;
    file.seekg(0, std::ios::end);
    file_size = file.tellg();
    file.seekg(0, std::ios::beg);
    result.reserve(file_size);
    result.insert(
        result.begin(),
        std::istream_iterator<uint8_t>(file),
        std::istream_iterator<uint8_t>());
    file.close();
    return result;
}

void Render_Engine::empty_deletion_queues(uint64_t frame)
{
    auto buffer_range = std::ranges::remove_if(m_buffer_deletion_queue, [this, frame](auto& element) {
        if (frame >= element.frame)
        {
            auto buffer = m_buffers[element.resource];
            buffer.resource->Release();
            m_buffers.remove(element.resource);
            return true;
        }
        return false;
        });
    m_buffer_deletion_queue.erase(buffer_range.begin(), buffer_range.end());

    auto texture_range = std::ranges::remove_if(m_texture_deletion_queue, [this, frame](auto& element) {
        if (frame >= element.frame)
        {
            auto texture = m_textures[element.resource];
            texture.resource->Release();
            m_textures.remove(element.resource);
            return true;
        }
        return false;
        });
    m_texture_deletion_queue.erase(texture_range.begin(), texture_range.end());

    auto pipeline_range = std::ranges::remove_if(m_pipeline_deletion_queue, [this, frame](auto& element) {
        if (frame >= element.frame)
        {
            auto pso = m_pipelines[element.resource];
            pso.pso->Release();
            m_pipelines.remove(element.resource);
            return true;
        }
        return false;
        });
    m_pipeline_deletion_queue.erase(pipeline_range.begin(), pipeline_range.end());
}
}
