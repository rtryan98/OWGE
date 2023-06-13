#include "owge_d3d12_base/d3d12_swapchain.hpp"

#include "owge_d3d12_base/d3d12_util.hpp"

#include <cassert>

namespace owge
{
D3D12_Swapchain::D3D12_Swapchain(IDXGIFactory4* factory,
    ID3D12Device* device, ID3D12CommandQueue* direct_queue, HWND hwnd, uint32_t buffer_count)
    : m_swapchain(), m_device(device), m_direct_queue(direct_queue), m_hwnd(hwnd), m_buffers()
{
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
    Com_Ptr<IDXGISwapChain1> swapchain1 = {};
    throw_if_failed(factory->CreateSwapChainForHwnd(m_direct_queue, hwnd,
        &swapchain_desc, nullptr, nullptr, swapchain1.GetAddressOf()),
        "Error creating Swapchain.");
    throw_if_failed(swapchain1->QueryInterface(m_swapchain.GetAddressOf()),
        "Error querying Swapchain Interface.");

    D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = {
        .Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
        .NumDescriptors = DXGI_MAX_SWAP_CHAIN_BUFFERS,
        .Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
        .NodeMask = 0
    };
    throw_if_failed(m_device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&m_descriptor_heap)),
        "Error creating Descriptor Heap for Swapchain RTVs.");

    recreate_resources();
}

void D3D12_Swapchain::acquire_next_image()
{
    m_current_idx = m_swapchain->GetCurrentBackBufferIndex();
}

bool D3D12_Swapchain::try_resize()
{
    DXGI_SWAP_CHAIN_DESC1 desc;
    throw_if_failed(m_swapchain->GetDesc1(&desc),
        "Error acquiring Swapchain description for resize.");
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
        assert(wait_idle_result == WAIT_OBJECT_0); wait_idle_result; // HACK: need custom assert macro
        m_buffers = {};
        throw_if_failed(m_swapchain->ResizeBuffers(0, client_width, client_height, DXGI_FORMAT_UNKNOWN, desc.Flags),
            "Error resizing Swapchain buffers.");
        recreate_resources();
    }
    return resize;
}

D3D12_Swapchain_Resources D3D12_Swapchain::get_acquired_resources() const
{
    return {
        .buffer = m_buffers[m_current_idx].Get(),
        .rtv_descriptor = m_descriptors[m_current_idx]
    };
}

IDXGISwapChain4* D3D12_Swapchain::get_swapchain() const
{
    return m_swapchain.Get();
}

void D3D12_Swapchain::recreate_resources()
{
    auto descriptor_cpu_handle_start = m_descriptor_heap->GetCPUDescriptorHandleForHeapStart();
    auto descriptor_increment = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    DXGI_SWAP_CHAIN_DESC1 desc;
    throw_if_failed(m_swapchain->GetDesc1(&desc),
        "Error acquiring Swapchain description for resource recreation.");

    for (uint32_t i = 0; i < desc.BufferCount; ++i)
    {
        auto descriptor_cpu_handle = descriptor_cpu_handle_start;
        descriptor_cpu_handle.ptr += i * descriptor_increment;
        throw_if_failed(m_swapchain->GetBuffer(i, IID_PPV_ARGS(&m_buffers[i])),
            "Error acquiring Swapchain Buffer.");
        D3D12_RENDER_TARGET_VIEW_DESC rtv_desc = {
            .Format = desc.Format,
            .ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D,
            .Texture2D = {
                .MipSlice = 0,
                .PlaneSlice = 0
            }
        };
        m_device->CreateRenderTargetView(m_buffers[i].Get(), &rtv_desc, descriptor_cpu_handle);
        m_descriptors[i] = descriptor_cpu_handle;
    }
}
}
