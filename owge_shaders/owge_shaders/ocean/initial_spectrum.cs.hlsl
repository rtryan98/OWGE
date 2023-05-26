#include "owge_shaders/bindless.hlsli"
#include "owge_shaders/ocean/oceanography.hlsli"

struct Ocean_Parameters
{
    float gravity;      // g
    float wind_speed;   // U
    float ocean_depth;  // h
    float fetch;        // F
    float lengthscale;  // L
    float2 size;        // N = M = 2^n
};

struct Bindset
{
    Raw_Buffer params;
    RW_Texture spectrum_tex;
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
    Ocean_Parameters pars = bnd.params.load_uniform<Ocean_Parameters>();

    float delta_k = 2.0f * mc_pi / pars.lengthscale;
    float2 k = (float2(id.x, id.y) - (pars.size / 2.0f)) * delta_k;
    float k_len = length(k);

    float omega = oceanography_dispersion_capillary(k_len, pars.gravity, pars.ocean_depth);
    float omega_d_dk = oceanography_dispersion_capillary_d_dk(k_len, pars.gravity, pars.ocean_depth);
    float omega_peak = oceanography_jonswap_omega_peak(pars.gravity, pars.wind_speed, pars.fetch);
    float theta = atan2(k.y, k.x);

    float non_directional_spectrum = oceanography_tma_spectrum(
        omega, omega_peak, pars.wind_speed, pars.gravity, pars.fetch, pars.ocean_depth);
    float directional_spectrum = oceanography_donelan_banner_directional_spreading(
        omega, omega_peak, theta);
    float spectrum = non_directional_spectrum * directional_spectrum;
    spectrum = sqrt(2.0f * spectrum * abs(omega_d_dk / omega) * pow(delta_k, 2.0f));
    float2 noise = float2(1.0f, 1.0f);
    bnd.spectrum_tex.store_2d(id.xy, noise * spectrum);
}
