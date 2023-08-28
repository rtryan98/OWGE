#include "owge_shaders/ocean/surface_render.hlsli"

float2 calculate_slope(float z_dx, float z_dy, float x_dx, float y_dy)
{
    return float2(z_dx / max(1.0 + x_dx, 0.0001), z_dy / max(1.0 + y_dy, 0.0001));
}

float3 calculate_normals(float2 slope)
{
    return normalize(float3(-slope.x, -slope.y, 1.0));
}

PS_Out ps_main(PS_In ps_in)
{
    PS_Out result;
    Bindset bnd = read_bindset_uniform<Bindset>(pc.bindset_buffer, pc.bindset_offset);
    Render_Data render_data = bnd.render_data.load<Render_Data>();

    float4 derivatives = 0.0;
    float jacobian = 0.0;
    [unroll(OCEAN_MAX_CASCADES)] for (uint i = 0; i < OCEAN_MAX_CASCADES; i++)
    {
        derivatives += bnd.displacement.sample_2d_array<float4>(bnd.surface_sampler.as_uniform(), ps_in.uvs[i], i); // z_dx, z_dy, x_dx, y_dy
        jacobian += bnd.jacobian.sample_2d_array<float>(bnd.surface_sampler.as_uniform(), ps_in.uvs[i], i);
    }

    float2 slope = calculate_slope(derivatives.z, derivatives.w, derivatives.x, derivatives.y);
    slope = calculate_slope(derivatives.x, derivatives.y, derivatives.z, derivatives.w);
    float3 normals = calculate_normals(slope);

    float3 light_dir = normalize(float3(0.0, 1.0, 0.5));

    float3 view_dir = normalize(render_data.camera_pos.xyz - ps_in.pos_ws);
    float3 refl_dir = normalize(reflect(-light_dir, normals));

    float ndotl = max(0.0, dot(normals, light_dir));

    float3 color = float3(0.0, 0.2156, 0.5686);


    float3 ambient = 0.05 * color;
    float3 specular = pow(max(dot(view_dir, refl_dir), 0.0), 5.0) * 0.25;
    float3 diffuse = ndotl * color;

    result.col = float4(ambient + specular + diffuse, 1.0);

    return result;
}
