#pragma once

#include <cstdint>
#include <d3d12.h>

namespace owge
{
template<typename T>
struct Base_Resource_Handle
{
    uint64_t alive : 1;
    uint64_t flags : 8;
    uint64_t gen : 23;
    uint64_t idx : 32;

    [[nodiscard]] bool operator==(Base_Resource_Handle other) const
    {
        return (alive == other.alive)
            && (flags == other.flags)
            && (idx == other.idx)
            && (gen == other.gen);
    }
    [[nodiscard]] bool operator!=(Base_Resource_Handle other) const
    {
        return !(*this == other);
    }
};

struct Buffer_Desc
{
    uint64_t size;
    D3D12_HEAP_TYPE heap_type;
    D3D12_BARRIER_LAYOUT initial_layout;
};

struct Buffer
{
    ID3D12Resource2* resource;
    uint32_t srv;
    uint32_t uav;
};
using Buffer_Handle = Base_Resource_Handle<Buffer>;

struct Texture_Desc
{

};

struct Texture
{
    ID3D12Resource2* resource;
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_dsv;
    uint32_t srv;
    uint32_t uav;
};
using Texture_Handle = Base_Resource_Handle<Texture>;

struct Pipeline_Desc
{

};

struct Pipeline
{
    ID3D12PipelineState* resource;
};
using Pipeline_Handle = Base_Resource_Handle<Pipeline>;
}
