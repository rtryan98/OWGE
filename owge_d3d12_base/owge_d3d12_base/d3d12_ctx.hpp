#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>

namespace owge
{
struct D3D12_Context_Settings
{
    bool enable_validation;
    bool enable_gpu_based_validation;
    bool disable_tdr;
};

struct D3D12_Context
{
    IDXGIFactory7* factory;
    IDXGIAdapter4* adapter;
    ID3D12Device12* device;
    ID3D12CommandQueue* direct_queue;
    ID3D12CommandQueue* async_compute_queue;
    ID3D12CommandQueue* copy_queue;
    ID3D12DescriptorHeap* cbv_srv_uav_descriptor_heap;
    ID3D12DescriptorHeap* sampler_descriptor_heap;
};

[[nodiscard]] D3D12_Context create_d3d12_context(const D3D12_Context_Settings* settings);
void destroy_d3d12_context(D3D12_Context* ctx);
}
