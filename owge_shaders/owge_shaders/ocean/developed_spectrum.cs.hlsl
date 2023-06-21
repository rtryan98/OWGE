#include "owge_shaders/bindless.hlsli"

struct Bindset
{
    Raw_Buffer params;
    RW_Texture spectrum_tex;
    RW_Texture angular_frequency_tex;
    RW_Texture developed_spectrum_tex;
    float time;
};

struct Push_Constants
{
    uint bindset_buffer;
    uint bindset_index;
    uint __pad0;
    uint __pad1;
};
ConstantBuffer<Push_Constants> pc : register(b0, space0);

[numthreads(32, 32, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    Bindset bnd = read_bindset_uniform<Bindset>(pc.bindset_buffer, pc.bindset_index);

    float2 spectrum = bnd.spectrum_tex.load_2d<float2>(id.xy);
    float omega_k = bnd.angular_frequency_tex.load_2d<float>(id.xy);

    bnd.developed_spectrum_tex.store_2d<float2>(id.xy, spectrum);
}
