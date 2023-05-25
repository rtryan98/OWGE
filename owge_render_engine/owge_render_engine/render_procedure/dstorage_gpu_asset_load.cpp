#include "owge_render_engine/render_procedure/dstorage_gpu_asset_load.hpp"

#include "owge_d3d12_base/d3d12_util.hpp"

#include <algorithm>
#include <array>
#include <ranges>

namespace owge
{
constexpr static uint32_t STATUS_ARRAY_SIZE = 128;
constexpr static uint32_t QUEUE_SIZE = 256;

DStorage_GPU_Asset_Load::DStorage_GPU_Asset_Load(IDStorageFactory* dstorage_factory, ID3D12Device* device)
{
    DSTORAGE_QUEUE_DESC queue_desc = {
        .SourceType = DSTORAGE_REQUEST_SOURCE_FILE,
        .Capacity = QUEUE_SIZE,
        .Priority = DSTORAGE_PRIORITY_NORMAL,
        .Name = nullptr,
        .Device = device
    };
    throw_if_failed(dstorage_factory->CreateQueue(&queue_desc, IID_PPV_ARGS(&m_file_queue)),
        "Error creating DStorageQueue for DStorage_GPU_Asset_Load.");

    throw_if_failed(dstorage_factory->CreateStatusArray(
        STATUS_ARRAY_SIZE, nullptr, IID_PPV_ARGS(&m_status_array)),
        "Error creating DStorageStatusArray for DStorage_GPU_Asset_Load.");
}

void DStorage_GPU_Asset_Load::process(ID3D12GraphicsCommandList9* cmd)
{
    std::vector<std::vector<D3D12_BUFFER_BARRIER>> buffer_barrier_groups;
    std::vector<std::vector<D3D12_TEXTURE_BARRIER>> texture_barrier_groups;
    std::vector<D3D12_BARRIER_GROUP> barrier_groups;

    barrier_groups.reserve(2 * m_payloads.size());
    buffer_barrier_groups.reserve(m_payloads.size());
    texture_barrier_groups.reserve(m_payloads.size());

    auto removed_elements = std::ranges::remove_if(m_payloads.begin(), m_payloads.end(),
        [this, &buffer_barrier_groups, &texture_barrier_groups, &barrier_groups]
        (DStorage_GPU_Asset_Load_Payload& payload) -> bool {
            bool removed = false;
            if (m_status_array->IsComplete(payload.status_array_index))
            {
                removed = true;

                auto& buffer_barriers = buffer_barrier_groups.emplace_back(std::move(payload.buffer_barriers));
                auto& buffer_barrier_group = barrier_groups.emplace_back();
                buffer_barrier_group.Type = D3D12_BARRIER_TYPE_BUFFER;
                buffer_barrier_group.NumBarriers = uint32_t(buffer_barriers.size());
                buffer_barrier_group.pBufferBarriers = buffer_barriers.data();

                auto& texture_barriers = texture_barrier_groups.emplace_back(std::move(payload.texture_barriers));
                auto& texture_barrier_group = barrier_groups.emplace_back();
                texture_barrier_group.Type = D3D12_BARRIER_TYPE_TEXTURE;
                texture_barrier_group.NumBarriers = uint32_t(texture_barriers.size());
                texture_barrier_group.pTextureBarriers = texture_barriers.data();
            }
            return removed;
        });
    m_payloads.erase(removed_elements.begin(), removed_elements.end());

    cmd->Barrier(uint32_t(barrier_groups.size()), barrier_groups.data());
}

void DStorage_GPU_Asset_Load::upload_resources(std::span<GPU_Upload_Request>&& requests)
{
    uint32_t status_index = get_next_status_index();
    for (const auto& request : requests)
    {
        upload_resource(request.dstorage_request, request.resource_states);
    }
    m_file_queue->EnqueueStatus(m_status_array.Get(), status_index);
    m_file_queue->Submit();
}

uint32_t DStorage_GPU_Asset_Load::get_next_status_index()
{
    return 0;
}

void DStorage_GPU_Asset_Load::upload_resource(const DSTORAGE_REQUEST& request, const Resource_State_After_Load& resource)
{
    resource;
    switch (request.Options.DestinationType)
    {
    case DSTORAGE_REQUEST_DESTINATION_BUFFER:
    {
        break;
    }
    case DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION:
    {
        break;
    }
    case DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES:
    {
        break;
    }
    case DSTORAGE_REQUEST_DESTINATION_MEMORY:
        [[fallthrough]];
    case DSTORAGE_REQUEST_DESTINATION_TILES:
        [[fallthrough]];
    default:
        break;
    }
}
}
