#pragma once

#include <array>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

namespace owge
{
class D3D12_Swapchain
{
public:
    D3D12_Swapchain(IDXGIFactory4* factory, ID3D12Device* device,
        ID3D12CommandQueue* direct_queue, HWND hwnd, uint32_t buffer_count);

    bool try_resize();

private:
    void recreate_resources();

private:
    Microsoft::WRL::ComPtr<IDXGISwapChain4> m_swapchain;
    ID3D12Device* m_device;
    ID3D12CommandQueue* m_direct_queue;
    HWND m_hwnd;
    std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, DXGI_MAX_SWAP_CHAIN_BUFFERS> m_buffers;
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_descriptor_heap;
    std::array<D3D12_CPU_DESCRIPTOR_HANDLE, DXGI_MAX_SWAP_CHAIN_BUFFERS> m_descriptors;
};
}
