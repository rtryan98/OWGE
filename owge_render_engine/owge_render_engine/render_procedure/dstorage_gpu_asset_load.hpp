#pragma once

#include "owge_d3d12_base/com_ptr.hpp"

#include "owge_render_engine/render_procedure/render_procedure.hpp"

#include <dstorage.h>
#include <span>
#include <vector>

namespace owge
{
struct DStorage_GPU_Asset_Load_Payload
{
    uint32_t status_array_index;
    std::vector<D3D12_BUFFER_BARRIER> buffer_barriers;
    std::vector<D3D12_TEXTURE_BARRIER> texture_barriers;
};

struct Buffer_State_After_Load
{
    D3D12_BARRIER_SYNC sync_after;
    D3D12_BARRIER_ACCESS access_after;
};

struct Texture_State_After_Load
{
    D3D12_BARRIER_SYNC sync_after;
    D3D12_BARRIER_ACCESS access_after;
    D3D12_BARRIER_LAYOUT layout_after;
    D3D12_BARRIER_SUBRESOURCE_RANGE range;
    D3D12_TEXTURE_BARRIER_FLAGS flags;
};

union Resource_State_After_Load
{
    Buffer_State_After_Load buffer;
    Texture_State_After_Load texture;
};

struct GPU_Upload_Request
{
    DSTORAGE_REQUEST dstorage_request;
    Resource_State_After_Load resource_states;
};

class DStorage_GPU_Asset_Load : public Render_Procedure
{
public:
    DStorage_GPU_Asset_Load(IDStorageFactory* dstorage_factory, ID3D12Device* device);
    virtual ~DStorage_GPU_Asset_Load() = default;

    virtual void process(ID3D12GraphicsCommandList9* cmd) override;

    void upload_resources(std::span<GPU_Upload_Request>&& requests);

private:
    [[nodiscard]] uint32_t get_next_status_index();

    void upload_resource(const DSTORAGE_REQUEST& request, const Resource_State_After_Load& resource);

private:
    std::vector<DSTORAGE_REQUEST> m_memory_requests = {};
    Com_Ptr<IDStorageQueue1> m_file_queue = {};
    Com_Ptr<IDStorageStatusArray> m_status_array = {};
    std::vector<DStorage_GPU_Asset_Load_Payload> m_payloads = {};
};
}
