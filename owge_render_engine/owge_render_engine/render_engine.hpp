#pragma once

#include <owge_d3d12_base/d3d12_ctx.hpp>
#include <owge_d3d12_base/d3d12_swapchain.hpp>

#include <memory>

namespace owge
{
class Window;

class Render_Engine
{
public:
    Render_Engine(const Window& window, const D3D12_Context_Settings& d3d12_context_settings);
    ~Render_Engine();

    // Delete special member functions. An instance of this can't be copied nor moved.
    Render_Engine(const Render_Engine&) = delete;
    Render_Engine(Render_Engine&&) = delete;
    Render_Engine& operator=(const Render_Engine&) = delete;
    Render_Engine& operator=(Render_Engine&&) = delete;

private:
    D3D12_Context m_ctx;
    std::unique_ptr<D3D12_Swapchain> m_swapchain;
};
}
