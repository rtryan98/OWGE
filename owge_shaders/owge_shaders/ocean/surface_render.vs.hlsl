#include "owge_shaders/ocean/surface_render.hlsli"
#include "owge_shaders/bindless.hlsli"

struct Vertex
{
    float2 pos;
};

VS_Out vs_main(uint vertex_id : SV_VertexID)
{
    VS_Out result;
    Bindset bnd = read_bindset_uniform<Bindset>(pc.bindset_buffer, pc.bindset_offset);
    Vertex vert = bnd.vertex_buffer.load<Vertex>(vertex_id);
    Render_Data render_data = bnd.render_data.load<Render_Data>();

    float4 pos = float4((vert.pos - 4096.0 / 2.0) * 0.03125, 0.0, 1.0);

    [unroll(OCEAN_MAX_CASCADES)] for (uint i = 0; i < OCEAN_MAX_CASCADES; i++)
    {
        result.uvs[i] = float2(pos.xy / render_data.length_scales[i]);
        pos += bnd.displacement.sample_level_2d_array<float4>(bnd.surface_sampler.as_uniform(), result.uvs[i], 0.0, i);
    }

    result.pos_ws = pos.xyz;
    result.pos = mul(pos, render_data.view_proj);

    return result;
}
