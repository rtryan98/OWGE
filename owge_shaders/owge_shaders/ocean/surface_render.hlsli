#ifndef OWGE_OCEAN_SURFACE_RENDER
#define OWGE_OCEAN_SURFACE_RENDER

struct VS_Out
{
    float4 pos : SV_POSITION;
};

typedef VS_Out PS_In;

struct PS_Out
{
    float4 col : SV_TARGET;
};

struct Push_Constants
{
    uint vs_bindset_buffer;
    uint vs_bindset_offset;
    uint ps_bindset_buffer;
    uint ps_bindset_offset;
};
ConstantBuffer<Push_Constants> pc : register(b0, space0);

#endif
