#pragma once

#include <d3d12.h>
#include <vector>

namespace owge
{
struct Staging_Buffer_Allocation
{
    ID3D12Resource* resource;
    uint64_t offset;
    void* data;
};

class Staging_Buffer_Allocator
{
public:
    Staging_Buffer_Allocator(ID3D12Device12* device);

    [[nodiscard]] Staging_Buffer_Allocation allocate(uint64_t size, uint64_t alignment = 0);
    void reset();

private:
    ID3D12Device12* m_device;
    uint64_t m_current_offset = 0;
    ID3D12Resource* m_current_resource = nullptr;
    void* m_mapped_data = nullptr;
    std::vector<ID3D12Resource*> m_resources = {};
};
}