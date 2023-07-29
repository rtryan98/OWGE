#include "owge_shaders/ocean/surface_render.hlsli"

struct Bindset
{
    uint a;
};

PS_Out ps_main(PS_In ps_in)
{
    PS_Out result;

    result.col = float4(1.0, 0.5, 0.5, 1.0);

    return result;
}
