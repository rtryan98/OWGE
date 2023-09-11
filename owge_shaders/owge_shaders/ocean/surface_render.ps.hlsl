#include "owge_shaders/ocean/surface_render.hlsli"

float2 calculate_slope(float z_dx, float z_dy, float x_dx, float y_dy)
{
    return float2(z_dx / (1.0 + /* lambda * */ x_dx), z_dy / (1.0 + /* lambda * */ y_dy));
}

float3 calculate_normals(float2 slope)
{
    // float3 dir = cross(float3(slope.x, 0.0, 0.0), float3(0.0, slope.y, 0.0));
    // return normalize(dir - float3(slope.x, slope.y, 0.0));
    return normalize(float3(-slope.x, -slope.y, 1.0));
}

float schlick_fresnel(float3 n, float3 v)
{
    return pow(saturate(1.0 - dot(n, v)), 5.0);
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
    float3 normals = calculate_normals(slope);

    float3 light_dir = normalize(float3(0.0, 0.0, 1.0));

    float3 view_dir = normalize(render_data.camera_pos.xyz - ps_in.pos_ws);
    float3 refl_dir = normalize(reflect(-light_dir, normals));

    float ndotl = max(0.0, dot(normals, light_dir));
    float fresnel = schlick_fresnel(normals, view_dir);

    float3 color = float3(0, 0.08866, 0.29177);

    float jacobian_bias = 3.65;
    if (jacobian - jacobian_bias < 0.0)
    {
        color = float3(0.55, 0.55, 0.55);
    }

    float3 ambient = 0.15 * color;
    float3 specular = pow(max(dot(view_dir, refl_dir), 0.0), 5.0) * 0.125;
    // specular = 0;
    float3 diffuse = ndotl * color;

    result.col = float4(ambient + specular + diffuse, 1.0);
    // result.col = float4(0.5 + 0.5 * normals, 1.0);

    return result;
}
