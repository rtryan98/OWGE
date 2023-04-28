#include "owge_render_engine/render_engine.hpp"

#include <owge_window/window.hpp>

namespace owge
{
constexpr uint32_t MAX_CONCURRENT_GPU_FRAMES = 2;
constexpr uint32_t MAX_SWAPCHAIN_BUFFERS = MAX_CONCURRENT_GPU_FRAMES + 1;

Render_Engine::Render_Engine(const Window& window,
    const D3D12_Context_Settings& d3d12_context_settings)
    : m_ctx(), m_swapchain()
{
    m_ctx = create_d3d12_context(&d3d12_context_settings);
    m_swapchain = std::make_unique<D3D12_Swapchain>(
        m_ctx.factory, m_ctx.device, m_ctx.direct_queue,
        window.get_hwnd(), MAX_SWAPCHAIN_BUFFERS);
}

Render_Engine::~Render_Engine()
{
    destroy_d3d12_context(&m_ctx);
}
}
