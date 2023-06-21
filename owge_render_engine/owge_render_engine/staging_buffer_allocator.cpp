#include "owge_render_engine/staging_buffer_allocator.hpp"
#include "owge_render_engine/render_engine.hpp"

namespace owge
{
static constexpr uint64_t DEFAULT_BUFFER_SIZE = 16777216; // 16 MB // TODO: make value customizable?

Staging_Buffer_Allocator::Staging_Buffer_Allocator(ID3D12Device10* device, Render_Engine* render_engine)
    : m_render_engine(render_engine), m_device(device)
{}

Staging_Buffer_Allocation Staging_Buffer_Allocator::allocate(uint64_t size, uint64_t alignment)
{
    D3D12_HEAP_PROPERTIES heap_properties = {
        .Type = D3D12_HEAP_TYPE_UPLOAD,
        .CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
        .MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
        .CreationNodeMask = 0,
        .VisibleNodeMask = 0
    };
    D3D12_RESOURCE_DESC1 resource_desc = {
        .Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
        .Alignment = 0,
        .Width = DEFAULT_BUFFER_SIZE,
        .Height = 1,
        .DepthOrArraySize = 1,
        .MipLevels = 1,
        .Format = DXGI_FORMAT_UNKNOWN,
        .SampleDesc = { .Count = 1, .Quality = 0 },
        .Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
        .Flags = D3D12_RESOURCE_FLAG_NONE,
        .SamplerFeedbackMipRegion = {}
    };

    if (size > DEFAULT_BUFFER_SIZE) // Handle overallocation
    {
        resource_desc.Width = size;
        void* mapped_data = nullptr;
        auto& new_resource = m_resources.emplace_back();
        m_device->CreateCommittedResource3(
            &heap_properties, D3D12_HEAP_FLAG_NONE,
            &resource_desc, D3D12_BARRIER_LAYOUT_UNDEFINED,
            nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&new_resource));
        new_resource->SetName(L"Staging_Buffer:Large_Upload_Buffer");
        new_resource->Map(0, nullptr, &mapped_data);
        return {
            .resource = new_resource,
            .offset = 0,
            .data = mapped_data
        };
    }
    if (m_current_resource == nullptr || m_current_offset + size > DEFAULT_BUFFER_SIZE)
    {
        auto& new_resource = m_resources.emplace_back();
        m_device->CreateCommittedResource3(
            &heap_properties, D3D12_HEAP_FLAG_NONE,
            &resource_desc, D3D12_BARRIER_LAYOUT_UNDEFINED,
            nullptr, nullptr, 0, nullptr, IID_PPV_ARGS(&new_resource));
        new_resource->SetName(L"Staging_Buffer:Small_Upload_Buffer");
        new_resource->Map(0, nullptr, &m_mapped_data);
        m_current_offset = 0;
        m_current_resource = new_resource;
    }
    Staging_Buffer_Allocation allocation = {
        .resource = m_current_resource,
        .offset = m_current_offset,
        .data = m_mapped_data
    };
    m_current_offset += size; // TODO: align.
    alignment;
    return allocation;
}

void Staging_Buffer_Allocator::reset()
{
    for (auto resource : m_resources)
    {
        m_render_engine->destroy_d3d12_resource_deferred(resource);
    }
    m_resources.clear();
    m_current_resource = nullptr;
}
}
