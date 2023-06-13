#include "owge_render_engine/command_allocator.hpp"

namespace owge
{
Com_Ptr<ID3D12CommandAllocator> create_command_allocator(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type)
{
    Com_Ptr<ID3D12CommandAllocator> allocator;
    device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator));
    return allocator;
}

Command_Allocator::Command_Allocator(ID3D12Device4* device, D3D12_COMMAND_LIST_TYPE type)
    : m_device(device), m_type(type)
{}

Command_Allocator::~Command_Allocator()
{
    for (auto& cmd_buffer : m_used)
    {
        cmd_buffer.cmd->Release();
        cmd_buffer.allocator->Release();
    }
    for (auto& cmd_buffer : m_recycled)
    {
        cmd_buffer.cmd->Release();
        cmd_buffer.allocator->Release();
    }
}

Command_Buffer Command_Allocator::get_or_allocate()
{
    Command_Buffer result = {};
    if (m_recycled.empty())
    {
        m_device->CreateCommandAllocator(m_type, IID_PPV_ARGS(&result.allocator));
        m_device->CreateCommandList1(0, m_type, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&result.cmd));
    }
    else
    {
        result = m_recycled.back();
        m_recycled.pop_back();
    }
    result.allocator->Reset();
    result.cmd->Reset(result.allocator, nullptr);
    m_used.push_back(result);
    return result;
}

void Command_Allocator::reset()
{
    m_recycled.insert(m_recycled.end(), m_used.begin(), m_used.end());
    m_used.clear();
}
}
