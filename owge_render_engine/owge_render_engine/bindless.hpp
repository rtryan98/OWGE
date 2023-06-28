#pragma once

#include "owge_render_engine/resource.hpp"

#include <cstdint>
#include <d3d12.h>
#include <vector>

namespace owge
{
static constexpr uint32_t MAX_BINDSET_VALUES = 16;

struct Bindset_Allocation
{
    uint32_t bindless_idx;
    uint32_t offset;
    uint32_t index;
    Buffer_Handle resource;
};

struct Buffer;
struct Texture;

struct Bindset
{
    void write_data(uint32_t first_element, uint32_t element_count, void* values);

    template<typename T>
    void write_data(const T& values)
    {
        static_assert(sizeof(T) <= sizeof(uint32_t) * MAX_BINDSET_VALUES);
        memcpy(data, &values, sizeof(T));
    }

    Bindset_Allocation allocation;
    uint32_t data[MAX_BINDSET_VALUES];
};

class Render_Engine;

class Bindset_Allocator
{
public:
    Bindset_Allocator(Render_Engine* render_engine);

    void release_resources();
    [[nodiscard]] Bindset allocate_bindset();
    void delete_bindset(const Bindset& bindset);

private:
    Render_Engine* m_render_engine;
    std::vector<Bindset_Allocation> m_freelist;
    std::vector<Buffer_Handle> m_resources;
    uint32_t m_current_index;
};

class Staging_Buffer_Allocator;

class Bindset_Stager
{
public:
    void stage_bindset(Render_Engine* render_engine, const Bindset& bindset, Staging_Buffer_Allocator* staging_buffer_allocator);
    void process(ID3D12GraphicsCommandList7* cmd);

private:
    struct Bindset_Staged_Allocation
    {
        ID3D12Resource* src;
        ID3D12Resource* dst;
        uint64_t src_offset;
        uint64_t dst_offset;
    };
    std::vector<Bindset_Staged_Allocation> m_bindset_staged_copies;
};
}
