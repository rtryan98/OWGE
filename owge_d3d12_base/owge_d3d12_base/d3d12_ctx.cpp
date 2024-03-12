#include "owge_d3d12_base/d3d12_ctx.hpp"

#include "owge_d3d12_base/com_ptr.hpp"
#include "owge_d3d12_base/d3d12_util.hpp"

#include <cassert>
#include <cstdint>
#include <dxgidebug.h>
#include <wrl.h>

extern "C" __declspec(dllexport) extern const uint32_t D3D12SDKVersion = 610;
extern "C" __declspec(dllexport) extern const char* D3D12SDKPath = ".\\D3D12\\";

namespace owge
{
ID3D12CommandQueue* create_d3d12_command_queue(ID3D12Device* device,
    D3D12_COMMAND_LIST_TYPE type, bool disable_tdr)
{
    D3D12_COMMAND_QUEUE_DESC desc = {
        .Type = type,
        .Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL,
        .Flags = disable_tdr
            ? D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT
            : D3D12_COMMAND_QUEUE_FLAG_NONE,
        .NodeMask = 0
    };
    ID3D12CommandQueue* result = nullptr;
    throw_if_failed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&result)),
        "Error creating Command Queue.");
    return result;
}

D3D12_Context create_d3d12_context(const D3D12_Context_Settings* settings)
{
    D3D12_Context ctx = {};

    // Create DXGIFactory
    uint32_t factory_flags = 0;
    if (settings->enable_validation)
    {
        factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        Com_Ptr<ID3D12Debug6> debug = {};
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();
            if (settings->enable_gpu_based_validation)
            {
                debug->SetEnableGPUBasedValidation(TRUE);
            }
        }
    }
    throw_if_failed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&ctx.factory)),
        "Error creating DXGI Factory.");

    // Create DXGIAdapter
    throw_if_failed(ctx.factory->EnumAdapterByGpuPreference(
        0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&ctx.adapter)),
        "Error enumerating Adapters.");

    // Create ID3D12Device
    throw_if_failed(D3D12CreateDevice(ctx.adapter, settings->d3d_feature_level, IID_PPV_ARGS(&ctx.device)),
        "Error creating Device.");

    // Create ID3D12CommandQueues
    ctx.direct_queue = create_d3d12_command_queue(ctx.device, D3D12_COMMAND_LIST_TYPE_DIRECT, settings->disable_tdr);
    ctx.async_compute_queue = create_d3d12_command_queue(ctx.device, D3D12_COMMAND_LIST_TYPE_COMPUTE, settings->disable_tdr);
    ctx.copy_queue = create_d3d12_command_queue(ctx.device, D3D12_COMMAND_LIST_TYPE_COPY, settings->disable_tdr);

    // Create ID3D12DescriptorHeaps
    D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
        .NumDescriptors = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
        .NodeMask = 0
    };
    throw_if_failed(ctx.device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&ctx.cbv_srv_uav_descriptor_heap)),
        "Error creating CBV_SRV_UAV Descriptor Heap.");
    descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    descriptor_heap_desc.NumDescriptors = D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE;
    throw_if_failed(ctx.device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&ctx.sampler_descriptor_heap)),
        "Error creating Sampler Descriptor Heap.");
    descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    descriptor_heap_desc.NumDescriptors = MAX_RTV_DSV_DESCRIPTORS;
    descriptor_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    throw_if_failed(ctx.device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&ctx.rtv_descriptor_heap)),
        "Error creating RTV Descriptor Heap.");
    descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    throw_if_failed(ctx.device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&ctx.dsv_descriptor_heap)),
        "Error creating DSV Descriptor Heap.");

    // D3D12_DESCRIPTOR_RANGE1 sampler_descriptor_range = {
    //     .RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER,
    //     .NumDescriptors = D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE,
    //     .BaseShaderRegister = 0,
    //     .RegisterSpace = 0,
    //     .Flags = D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE,
    //     .OffsetInDescriptorsFromTableStart = 0
    // };
    D3D12_ROOT_PARAMETER1 global_rootsig_params[] = {
        {
            .ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS,
            .Constants = {
                .ShaderRegister = 0,
                .RegisterSpace = 0,
                .Num32BitValues = 4
            },
            .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        },
        // {
        //     .ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
        //     .DescriptorTable = {
        //         .NumDescriptorRanges = 1,
        //         .pDescriptorRanges = &sampler_descriptor_range
        //     },
        //     .ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL
        // }
    };
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC versioned_rootsig_desc = {
        .Version = D3D_ROOT_SIGNATURE_VERSION_1_1,
        .Desc_1_2 = {
            .NumParameters = 1,
            .pParameters = global_rootsig_params,
            .NumStaticSamplers = UINT(settings->static_samplers.size()),
            .pStaticSamplers = settings->static_samplers.data(),
            .Flags = D3D12_ROOT_SIGNATURE_FLAG_CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED
                | D3D12_ROOT_SIGNATURE_FLAG_SAMPLER_HEAP_DIRECTLY_INDEXED
        }
    };
    Com_Ptr<ID3DBlob> rootsig_error_blob;
    Com_Ptr<ID3DBlob> rootsig_blob;
    throw_if_failed(D3D12SerializeVersionedRootSignature(
        &versioned_rootsig_desc, &rootsig_blob, &rootsig_error_blob),
        "Error serializing Root Signature.");
    throw_if_failed(ctx.device->CreateRootSignature(
        0, rootsig_blob->GetBufferPointer(), rootsig_blob->GetBufferSize(), IID_PPV_ARGS(&ctx.global_rootsig)),
        "Error creating Root Signature.");

    return ctx;
}

void destroy_d3d12_context(D3D12_Context* ctx)
{
    d3d12_context_wait_idle(ctx);

    ctx->global_rootsig->Release();
    ctx->dsv_descriptor_heap->Release();
    ctx->rtv_descriptor_heap->Release();
    ctx->sampler_descriptor_heap->Release();
    ctx->cbv_srv_uav_descriptor_heap->Release();
    ctx->copy_queue->Release();
    ctx->async_compute_queue->Release();
    ctx->direct_queue->Release();

    Com_Ptr<ID3D12DebugDevice> debug_device;
    bool has_debug_device = SUCCEEDED(ctx->device->QueryInterface(IID_PPV_ARGS(&debug_device)));

    ctx->device->Release();
    ctx->adapter->Release();
    ctx->factory->Release();
    if (has_debug_device)
    {
        debug_device->ReportLiveDeviceObjects(D3D12_RLDO_DETAIL | D3D12_RLDO_IGNORE_INTERNAL);
    }
}

void d3d12_context_wait_idle(D3D12_Context* ctx)
{
    DWORD result = wait_for_d3d12_queue_idle(ctx->device, ctx->direct_queue);
    assert(result == WAIT_OBJECT_0);
    result = wait_for_d3d12_queue_idle(ctx->device, ctx->async_compute_queue);
    assert(result == WAIT_OBJECT_0);
    result = wait_for_d3d12_queue_idle(ctx->device, ctx->copy_queue);
    assert(result == WAIT_OBJECT_0);
}
}
