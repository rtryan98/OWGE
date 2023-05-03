#pragma once

#include <d3d12.h>

namespace owge
{
class Render_Procedure
{
public:
    virtual ~Render_Procedure() = default;

    virtual void process(ID3D12GraphicsCommandList9* cmd) = 0;
};
}
