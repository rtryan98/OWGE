#include "owge_render_engine/render_engine.hpp"

namespace owge
{
static constexpr uint32_t MAX_CONCURRENT_GPU_FRAMES = 2;
static constexpr uint32_t MAX_SWAPCHAIN_BUFFERS = MAX_CONCURRENT_GPU_FRAMES + 1;

static constexpr uint32_t MAX_BUFFERS = 1048576;
static constexpr uint32_t MAX_TEXTURES = 1048576;
static constexpr uint32_t MAX_PIPELINES = 65536;

static constexpr uint32_t NO_SRV = ~0u;
static constexpr uint32_t NO_UAV = ~0u;

Render_Engine::Render_Engine(HWND hwnd,
    const D3D12_Context_Settings& d3d12_context_settings)
    : m_ctx(),
    m_swapchain(),
    m_buffers(MAX_BUFFERS),
    m_textures(MAX_TEXTURES),
    m_pipelines(MAX_PIPELINES)
{
    m_ctx = create_d3d12_context(&d3d12_context_settings);
    m_swapchain = std::make_unique<D3D12_Swapchain>(
        m_ctx.factory, m_ctx.device, m_ctx.direct_queue,
        hwnd, MAX_SWAPCHAIN_BUFFERS);
}

Render_Engine::~Render_Engine()
{
    destroy_d3d12_context(&m_ctx);
}

void Render_Engine::render()
{
    if (m_swapchain->try_resize())
    {
        // TODO: client window area dependent resources
    }
    m_swapchain->acquire_next_image();

    ID3D12GraphicsCommandList9* cmd{};

    auto swapchain_res = m_swapchain->get_acquired_resources();
    D3D12_TEXTURE_BARRIER swapchain_barrier = {
        .SyncBefore = D3D12_BARRIER_SYNC_NONE,
        .SyncAfter = D3D12_BARRIER_SYNC_RENDER_TARGET,
        .AccessBefore = D3D12_BARRIER_ACCESS_NO_ACCESS,
        .AccessAfter = D3D12_BARRIER_ACCESS_RENDER_TARGET,
        .LayoutBefore = D3D12_BARRIER_LAYOUT_UNDEFINED,
        .LayoutAfter = D3D12_BARRIER_LAYOUT_RENDER_TARGET,
        .pResource = swapchain_res.buffer,
        .Subresources = {
            .IndexOrFirstMipLevel = 0xFFFFFFFF,
            .NumMipLevels = 0,
            .FirstArraySlice = 0,
            .NumArraySlices = 0,
            .FirstPlane = 0,
            .NumPlanes = 0
        },
        .Flags = D3D12_TEXTURE_BARRIER_FLAG_NONE // Maybe DISCARD?
    };
    D3D12_BARRIER_GROUP swapchain_barrier_group = {
        .Type = D3D12_BARRIER_TYPE_TEXTURE,
        .NumBarriers = 1,
        .pTextureBarriers = &swapchain_barrier
    };

    cmd->Barrier(1, &swapchain_barrier_group);
    float clear_color[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    cmd->ClearRenderTargetView(swapchain_res.rtv_descriptor, clear_color, 0, nullptr);

    for (auto& procedure : m_procedures)
    {
        procedure->process(cmd);
    }

    swapchain_barrier.LayoutBefore = D3D12_BARRIER_LAYOUT_RENDER_TARGET;
    swapchain_barrier.LayoutAfter = D3D12_BARRIER_LAYOUT_PRESENT;
    cmd->Barrier(1, &swapchain_barrier_group);
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
    m_ctx.device->CreateCommittedResource3(
        &heap_properties, D3D12_HEAP_FLAG_NONE,
        &resource_desc, desc.initial_layout, nullptr,
        nullptr, 0, nullptr, IID_PPV_ARGS(&buffer.resource));

    buffer.srv = NO_SRV;
    buffer.uav = NO_UAV;

    return m_buffers.insert(0, buffer);
}
}
