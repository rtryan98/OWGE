#include "owge_shaders/ocean/surface_render.hlsli"
#include "owge_shaders/bindless.hlsli"

struct Render_Data
{
    float4x4 viewproj;
};

struct Bindset
{
    Array_Buffer vertex_buffer;
    Raw_Buffer render_data;
};

struct Vertex
{
    float2 pos;
};

VS_Out vs_main(uint vertex_id : SV_VertexID)
{
    VS_Out result;
    Bindset bnd = read_bindset_uniform<Bindset>(pc.vs_bindset_buffer, pc.vs_bindset_offset);
    Vertex vert = bnd.vertex_buffer.load<Vertex>(vertex_id);
    Render_Data render_data = bnd.render_data.load<Render_Data>();

    result.pos = mul(float4(vert.pos, 0.0, 1.0), render_data.viewproj);

    return result;
}
