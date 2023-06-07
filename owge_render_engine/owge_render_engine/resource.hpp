#pragma once

#include <cstddef>
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

enum class Resource_Usage
{
    Read_Only,
    Read_Write
};

struct Buffer_Desc
{
    uint64_t size;
    D3D12_HEAP_TYPE heap_type;
    D3D12_BARRIER_LAYOUT initial_layout;
    Resource_Usage usage;
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
    uint32_t width;
    uint32_t height;
    uint32_t depth_or_array_layers;
    uint32_t mip_levels;
    D3D12_RESOURCE_DIMENSION dimension;
    D3D12_SRV_DIMENSION srv_dimension;
    D3D12_UAV_DIMENSION uav_dimension;
    D3D12_RTV_DIMENSION rtv_dimension;
    D3D12_DSV_DIMENSION dsv_dimension;
    D3D12_BARRIER_LAYOUT initial_layout;
    DXGI_FORMAT format;
    Resource_Usage usage;
};

struct Texture
{
    ID3D12Resource2* resource;
    uint32_t rtv_dsv;
    uint32_t srv;
    uint32_t uav;
};
using Texture_Handle = Base_Resource_Handle<Texture>;

struct Graphics_Pipeline_Desc
{

};

struct Compute_Pipeline_Desc
{
    void* bytecode;
    std::size_t bytecode_length;
    void* cached_bytecode;
    std::size_t cached_bytecode_length;
};

struct Pipeline
{
    ID3D12PipelineState* resource;
};
using Pipeline_Handle = Base_Resource_Handle<Pipeline>;
}
