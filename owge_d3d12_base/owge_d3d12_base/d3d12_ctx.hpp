#pragma once

#include <include/d3d12.h>
#include <dxgi1_6.h>
#include <span>

namespace owge
{
static constexpr uint32_t IMGUI_DESCRIPTOR_INDEX = D3D12_MAX_SHADER_VISIBLE_DESCRIPTOR_HEAP_SIZE_TIER_2 - 1;

struct D3D12_Context_Settings
{
    bool enable_validation;
    bool enable_gpu_based_validation;
    bool disable_tdr;
    D3D_FEATURE_LEVEL d3d_feature_level;
    std::span<D3D12_STATIC_SAMPLER_DESC1> static_samplers;
};

struct D3D12_Context
{
    IDXGIFactory7* factory;
    IDXGIAdapter4* adapter;
    ID3D12Device10* device;
    ID3D12CommandQueue* direct_queue;
    ID3D12CommandQueue* async_compute_queue;
    ID3D12CommandQueue* copy_queue;
    ID3D12DescriptorHeap* cbv_srv_uav_descriptor_heap;
    ID3D12DescriptorHeap* sampler_descriptor_heap;
    ID3D12DescriptorHeap* rtv_descriptor_heap;
    ID3D12DescriptorHeap* dsv_descriptor_heap;
    ID3D12RootSignature* global_rootsig;
};

[[nodiscard]] D3D12_Context create_d3d12_context(const D3D12_Context_Settings* settings);
void destroy_d3d12_context(D3D12_Context* ctx);
void d3d12_context_wait_idle(D3D12_Context* ctx);
}
