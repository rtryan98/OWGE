#include "owge_d3d12_base/d3d12_ctx.hpp"

#include "owge_d3d12_base/d3d12_util.hpp"

#include <cstdint>
#include <wrl.h>

extern "C" __declspec(dllexport) const uint32_t D3D12SDKVersion = 610;
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
    throw_if_failed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&result)));
    return result;
}

D3D12_Context create_d3d12_context(const D3D12_Context_Settings* settings)
{
    using Microsoft::WRL::ComPtr;

    D3D12_Context ctx = {};

    // Create DXGIFactory
    uint32_t factory_flags = 0;
    if (settings->enable_validation)
    {
        factory_flags |= DXGI_CREATE_FACTORY_DEBUG;
        ComPtr<ID3D12Debug6> debug = {};
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
        {
            debug->EnableDebugLayer();
            if (settings->enable_gpu_based_validation)
            {
                debug->SetEnableGPUBasedValidation(TRUE);
            }
        }
    }
    throw_if_failed(CreateDXGIFactory2(factory_flags, IID_PPV_ARGS(&ctx.factory)));

    // Create DXGIAdapter
    throw_if_failed(ctx.factory->EnumAdapterByGpuPreference(
        0, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&ctx.adapter)));

    // Create ID3D12Device
    throw_if_failed(D3D12CreateDevice(ctx.adapter, settings->d3d_feature_level, IID_PPV_ARGS(&ctx.device)));

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
    throw_if_failed(ctx.device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&ctx.cbv_srv_uav_descriptor_heap)));
    descriptor_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
    descriptor_heap_desc.NumDescriptors = D3D12_MAX_SHADER_VISIBLE_SAMPLER_HEAP_SIZE;
    throw_if_failed(ctx.device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&ctx.sampler_descriptor_heap)));

    return ctx;
}

void destroy_d3d12_context(D3D12_Context* ctx)
{
    ctx->sampler_descriptor_heap->Release();
    ctx->cbv_srv_uav_descriptor_heap->Release();
    ctx->copy_queue->Release();
    ctx->async_compute_queue->Release();
    ctx->direct_queue->Release();
    ctx->device->Release();
    ctx->adapter->Release();
    ctx->factory->Release();
}
}
