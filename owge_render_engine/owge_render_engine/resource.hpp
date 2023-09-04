#pragma once

#include <cstddef>
#include <cstdint>
#include <include/d3d12.h>
#include <string>
#include <vector>

namespace owge
{
template<typename T>
struct Base_Resource_Handle
{
    uint64_t alive : 1;
    uint64_t flags : 3;
    uint64_t bindless_idx : 20;
    uint64_t gen : 20;
    uint64_t resource_idx : 20;

    [[nodiscard]] bool operator==(Base_Resource_Handle other) const
    {
        return (alive == other.alive)
            && (flags == other.flags)
            && (bindless_idx == other.bindless_idx)
            && (gen == other.gen)
            && (resource_idx == other.resource_idx);
    }
    [[nodiscard]] bool operator!=(Base_Resource_Handle other) const
    {
        return !(*this == other);
    }

    [[nodiscard]] bool is_null_handle() const
    {
        return alive == 0
            && flags == 0
            && bindless_idx == 0
            && gen == 0
            && resource_idx == 0;
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
    Resource_Usage usage;
};

struct Buffer
{
    ID3D12Resource2* resource;
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
    D3D12_CLEAR_VALUE optimized_clear_value;
};

struct Texture
{
    ID3D12Resource2* resource;
    uint32_t rtv;
    uint32_t dsv;
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
    uint32_t rtv_count;
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
    uint32_t workgroups_x;
    uint32_t workgroups_y;
    uint32_t workgroups_z;
    union
    {
        Graphics_Pipeline_Desc graphics;
        Compute_Pipeline_Desc compute;
    };
};
using Pipeline_Handle = Base_Resource_Handle<Pipeline>;

struct Sampler_Desc
{
    D3D12_FILTER filter;
    D3D12_TEXTURE_ADDRESS_MODE address_u;
    D3D12_TEXTURE_ADDRESS_MODE address_v;
    D3D12_TEXTURE_ADDRESS_MODE address_w;
    float mip_lod_bias;
    uint32_t max_anisotropy;
    D3D12_COMPARISON_FUNC comparison_func;
    float boder_color[4];
    float min_lod;
    float max_lod;
};

struct Sampler
{};
using Sampler_Handle = Base_Resource_Handle<Sampler>;
}
