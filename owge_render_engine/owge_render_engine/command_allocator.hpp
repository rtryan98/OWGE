#pragma once

#include <include/d3d12.h>
#include <owge_d3d12_base/com_ptr.hpp>
#include <vector>

namespace owge
{
struct Command_Buffer
{
    ID3D12CommandAllocator* allocator;
    ID3D12GraphicsCommandList7* cmd;
};

class Command_Allocator
{
public:
    Command_Allocator(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type);
    ~Command_Allocator();

    [[nodiscard]] Command_Buffer get_or_allocate();
    void reset();

private:
    ID3D12Device4* m_device;
    D3D12_COMMAND_LIST_TYPE m_type;
    std::vector<Command_Buffer> m_used = {};
    std::vector<Command_Buffer> m_recycled = {};
};
}
