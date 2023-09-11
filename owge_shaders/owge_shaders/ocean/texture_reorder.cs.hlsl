#include "owge_shaders/bindless.hlsli"

struct Bindset
{
    RW_Texture packed_x_y;
    RW_Texture packed_z_x_dx;
    RW_Texture packed_y_dx_z_dx;
    RW_Texture packed_y_dy_z_dy;

    RW_Texture displacement;
    RW_Texture derivatives; // z_dx, z_dy, x_dx, y_dy
    RW_Texture folding_map;
};

struct Push_Constants
{
    uint bindset_buffer;
    uint bindset_offset;
    uint __pad0;
    uint __pad1;
};
ConstantBuffer<Push_Constants> pc : register(b0, space0);

[numthreads(32, 32, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    Bindset bnd = read_bindset_uniform<Bindset>(pc.bindset_buffer, pc.bindset_offset);
    float2 x_y =        bnd.packed_x_y.load_2d_array<float2>(id.xyz);
    float2 z_x_dx =     bnd.packed_z_x_dx.load_2d_array<float2>(id.xyz);
    float2 y_dx_z_dx =  bnd.packed_y_dx_z_dx.load_2d_array<float2>(id.xyz);
    float2 y_dy_z_dy =  bnd.packed_y_dy_z_dy.load_2d_array<float2>(id.xyz);

    float x = x_y.x;
    float y = x_y.y;
    float z = z_x_dx.x;
    float x_dx = z_x_dx.y;
    float y_dx = y_dx_z_dx.x;
    float z_dx = y_dx_z_dx.y;
    // float x_dy = y_dy_z_dy.x; == y_dy
    float y_dy = y_dy_z_dy.x;
    float z_dy = y_dy_z_dy.y;

    float j_x_dx = 1.0 + /* lambda * */ x_dx;
    float j_y_dy = 1.0 + /* lambda * */ y_dy;
    float j_y_dx = /* lambda * */ y_dx;
    float j_x_dy = j_y_dx;
    float jacobian = j_x_dx * j_y_dy - j_x_dy * j_y_dx;

    bnd.displacement.store_2d_array(id, float4(x, y, z, 0.0));
    bnd.derivatives.store_2d_array(id, float4(z_dx, z_dy, x_dx, y_dy));
    bnd.folding_map.store_2d_array(id, jacobian);
}
