#pragma once

#include <d3d12.h>
#include <string>

namespace owge
{
class Render_Engine;
class Barrier_Builder;
class D3D12_Swapchain;

struct Render_Procedure_Payload
{
    Render_Engine* render_engine;
    ID3D12GraphicsCommandList9* cmd;
    Barrier_Builder* barrier_builder;
    D3D12_Swapchain* swapchain;
};

class Render_Procedure
{
public:
    virtual ~Render_Procedure() = default;

    virtual void process(const Render_Procedure_Payload& payload) = 0;

protected:
    Render_Procedure() = default;
    Render_Procedure(std::string_view name)
        : m_name( name )
    {}

private:
    std::string m_name;
};
}
