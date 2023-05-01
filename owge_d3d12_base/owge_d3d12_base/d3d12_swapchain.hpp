#pragma once

#include "owge_d3d12_base/com_ptr.hpp"

#include <array>
#include <d3d12.h>
#include <dxgi1_6.h>

namespace owge
{
struct D3D12_Swapchain_Resources
{
    ID3D12Resource* buffer;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_descriptor;
};

class D3D12_Swapchain
{
public:
    D3D12_Swapchain(IDXGIFactory4* factory, ID3D12Device* device,
        ID3D12CommandQueue* direct_queue, HWND hwnd, uint32_t buffer_count);

    void acquire_next_image();
    [[nodiscard]] bool try_resize();
    [[nodiscard]] D3D12_Swapchain_Resources get_acquired_resources() const;

private:
    void recreate_resources();

private:
    Com_Ptr<IDXGISwapChain4> m_swapchain;
    ID3D12Device* m_device;
    ID3D12CommandQueue* m_direct_queue;
    HWND m_hwnd;
    std::array<Com_Ptr<ID3D12Resource>, DXGI_MAX_SWAP_CHAIN_BUFFERS> m_buffers;
    Com_Ptr<ID3D12DescriptorHeap> m_descriptor_heap;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, DXGI_MAX_SWAP_CHAIN_BUFFERS> m_descriptors;
    uint32_t m_current_idx = {};
};
}
