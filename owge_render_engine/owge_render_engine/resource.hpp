#pragma once

#include <cstddef>
#include <cstdint>
#include <d3d12.h>
#include <vector>

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

    [[nodiscard]] bool is_null_handle() const
    {
        return alive == 0
            && flags == 0
            && gen == 0
            && idx == 0;
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
};

struct Texture
{
    ID3D12Resource2* resource;
    uint32_t rtv_dsv;
    uint32_t srv;
    uint32_t uav;
};
using Texture_Handle = Base_Resource_Handle<Texture>;

struct Shader_Desc
{
    std::string path;
};

struct Shader
{
    std::string path;
    std::vector<uint8_t> bytecode;
};
using Shader_Handle = Base_Resource_Handle<Shader>;

struct Graphics_Pipeline_Desc
{
    struct
    {
        Shader_Handle vs;
        Shader_Handle ps;
        Shader_Handle ds;
        Shader_Handle hs;
        Shader_Handle gs;
    } shaders;
    D3D12_PRIMITIVE_TOPOLOGY_TYPE primitive_topology_type;
    D3D12_BLEND_DESC blend_state;
    D3D12_RASTERIZER_DESC rasterizer_state;
    D3D12_DEPTH_STENCIL_DESC depth_stencil_state;
    DXGI_FORMAT rtv_formats[8];
    DXGI_FORMAT dsv_format;
};

struct Compute_Pipeline_Desc
{
    Shader_Handle cs;
};

enum class Pipeline_Type
{
    Graphics,
    Compute
};

struct Pipeline
{
    ID3D12PipelineState* pso;
    Pipeline_Type type;
    union
    {
        Graphics_Pipeline_Desc graphics;
        Compute_Pipeline_Desc compute;
    };
};
using Pipeline_Handle = Base_Resource_Handle<Pipeline>;
}
