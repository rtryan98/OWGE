#include "owge_d3d12_base/d3d12_swapchain.hpp"

#include "owge_d3d12_base/d3d12_util.hpp"

#include <cassert>

namespace owge
{
D3D12_Swapchain::D3D12_Swapchain(IDXGIFactory4* factory,
    ID3D12Device* device, ID3D12CommandQueue* direct_queue, HWND hwnd, uint32_t buffer_count)
    : m_swapchain(), m_device(device), m_direct_queue(direct_queue), m_hwnd(hwnd), m_buffers()
{
    using Microsoft::WRL::ComPtr;

    DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {
        .Format = DXGI_FORMAT_R8G8B8A8_UNORM, // TODO: make this customizable for HDR and sRGB?
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
        .BufferCount = buffer_count,
        .Scaling = DXGI_SCALING_STRETCH,
        .SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
        .AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED,
        .Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING // TODO: query support for tearing
    };
    ComPtr<IDXGISwapChain1> swapchain1 = {};
    throw_if_failed(factory->CreateSwapChainForHwnd(m_direct_queue, hwnd,
        &swapchain_desc, nullptr, nullptr, swapchain1.GetAddressOf()));
    throw_if_failed(swapchain1->QueryInterface(m_swapchain.GetAddressOf()));

    recreate_resources();
}

bool D3D12_Swapchain::try_resize()
{
    DXGI_SWAP_CHAIN_DESC1 desc;
    throw_if_failed(m_swapchain->GetDesc1(&desc));
    RECT rect;
    GetClientRect(m_hwnd, &rect);
    uint32_t client_width = rect.right - rect.left;
    uint32_t client_height = rect.bottom - rect.top;

    bool has_area = false;
    has_area = ( client_width > 0 ) && ( client_height > 0 );

    bool resize = false;
    resize = ((client_width != desc.Width) || (client_height != desc.Height)) && has_area;

    if (resize)
    {
        auto wait_idle_result = wait_for_d3d12_queue_idle(m_device, m_direct_queue);
        assert(wait_idle_result == WAIT_OBJECT_0);
        m_buffers = {};
        m_swapchain->ResizeBuffers(0, client_width, client_height, DXGI_FORMAT_UNKNOWN, 0);
        recreate_resources();
    }
    return resize;
}

void D3D12_Swapchain::recreate_resources()
{
    auto descriptor_cpu_handle = m_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    auto descriptor_increment = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    DXGI_SWAP_CHAIN_DESC1 desc;
    throw_if_failed(m_swapchain->GetDesc1(&desc));

    for (uint32_t i = 0; i < desc.BufferCount; ++i, descriptor_cpu_handle.ptr += descriptor_increment)
    {
        m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i]));
        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
            .Format = desc.Format,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            .Texture2D = {
                .MipSlice = 0,
                .PlaneSlice = 0
            }
        };
        m_device->CreateRenderTargetView(m_buffers[i].Get(), &rtv_desc, descriptor_cpu_handle);
    }
}
}
