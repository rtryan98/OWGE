#ifndef OWGE_OCEAN_SURFACE_RENDER
#define OWGE_OCEAN_SURFACE_RENDER

#include "owge_shaders/bindless.hlsli"

static const uint OCEAN_MAX_CASCADES = 4;

struct Render_Data
{
    float4x4 view_proj;
    float length_scales[OCEAN_MAX_CASCADES];
    float4 camera_pos;
};

struct Bindset
{
    Array_Buffer vertex_buffer;
    Raw_Buffer render_data;
    Texture displacement;
    Texture derivatives; // z_dx, z_dy, x_dx, y_dy
    Texture jacobian;
    Sampler surface_sampler;
};

struct VS_Out
{
    float4 pos : SV_POSITION;
    float2 uvs[OCEAN_MAX_CASCADES] : TEXCOORD0;
    float3 pos_ws : POSITION;
};

typedef VS_Out PS_In;

struct PS_Out
{
    float4 col : SV_TARGET;
};

struct Push_Constants
{
    uint bindset_buffer;
    uint bindset_offset;
    uint __pad0;
    uint __pad1;
};
ConstantBuffer<Push_Constants> pc : register(b0, space0);

#endif
