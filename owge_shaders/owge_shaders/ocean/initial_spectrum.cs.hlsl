#include "owge_shaders/bindless.hlsli"
#include "owge_shaders/complex.hlsli"
#include "owge_shaders/rand.hlsli"
#include "owge_shaders/ocean/oceanography.hlsli"

struct Spectrum_Parameters
{
    float wind_speed;                   // U
    float fetch;                        // F
    float v_yu_karaev_spectrum_omega_m; // 0.61826 + 0.0000003529 * F - 0.00197508 * sqrt(F) + (62.554 / sqrt(F)) - (290.2 / F)
};

struct Ocean_Parameters
{
    uint  size;                         // N = M = 2^n
    float lengthscale;                  // L
    float gravity;                      // g
    float ocean_depth;                  // h
    Spectrum_Parameters spectra[2];
};

struct Bindset
{
    Raw_Buffer params;
    RW_Texture spectrum_tex;
    RW_Texture angular_frequency_tex;
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
    Ocean_Parameters pars = bnd.params.load_uniform<Ocean_Parameters>();

    int2 id_shifted = int2(id.xy) - int2(pars.size, pars.size) / 2;
    float delta_k = (2.0f * MC_PI) / pars.lengthscale;
    float2 k = id_shifted * delta_k;
    float k_len = length(k);

    float omega = oceanography_dispersion_capillary(k_len, pars.gravity, pars.ocean_depth);
    float omega_d_dk = oceanography_dispersion_capillary_d_dk(k_len, pars.gravity, pars.ocean_depth);
    float omega_peak = oceanography_jonswap_omega_peak(pars.gravity, pars.spectra[0].wind_speed, pars.spectra[0].fetch);
    float theta = atan2(k.y, k.x);

    float non_directional_spectrum = oceanography_tma_spectrum(
        omega, omega_peak, pars.spectra[0].wind_speed, pars.gravity, pars.spectra[0].fetch, pars.ocean_depth);
    float directional_spectrum = oceanography_donelan_banner_directional_spreading(
        omega, omega_peak, theta);
    float spectrum = non_directional_spectrum * directional_spectrum;
    spectrum = sqrt(2.0f * spectrum * abs(omega_d_dk / k_len) * pow(delta_k, 2.0f));
    float2 final_spectrum = box_muller_22(hash_nosine_22(k), 0.0, 1.0) * spectrum;

    if(id_shifted.x == 0 && id_shifted.y == 0)
    {
        // We need to set the 0th wavevector to 0.
        // This does not break the calculation either, as this only corresponds to the DC-part of the spectrum.
        final_spectrum = float2(0.0, 0.0);
    }

    bnd.spectrum_tex.store_2d(id.xy, float4(final_spectrum, k));
    bnd.angular_frequency_tex.store_2d(id.xy, omega);
}
