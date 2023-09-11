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
    uint oceanographic_spectrum;
    float lengthscales[4];              // L
    float spectral_cutoffs_low[4];
    float spectral_cutoffs_high[4];
    float gravity;                      // g
    float ocean_depth;                  // h
    Spectrum_Parameters spectra[2];     // Local and Swell
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
    float delta_k = (2.0f * MC_PI) / pars.lengthscales[id.z];
    float2 k = id_shifted * delta_k;
    float wavenumber = length(k);

    float omega = oceanography_dispersion_capillary(wavenumber, pars.gravity, pars.ocean_depth);
    float omega_d_dk = oceanography_dispersion_capillary_d_dk(wavenumber, pars.gravity, pars.ocean_depth);
    float omega_peak = oceanography_jonswap_omega_peak(pars.gravity, pars.spectra[0].wind_speed, pars.spectra[0].fetch);
    float theta = atan2(k.y, k.x);

    float non_directional_spectrum = oceanography_tma_spectrum(
        omega, omega_peak, pars.spectra[0].wind_speed, pars.gravity, pars.spectra[0].fetch, pars.ocean_depth);
    float directional_spectrum = oceanography_donelan_banner_directional_spreading(
        omega, omega_peak, theta);

    float spectrum = non_directional_spectrum * directional_spectrum;
    spectrum = sqrt(2.0f * spectrum * abs(omega_d_dk / wavenumber) * delta_k * delta_k);
    float2 noise = 1.0 / sqrt(2.0) * float2(
        box_muller_12(1.0/float(0xffffffffu) * float2(pcg3d(id.xyz).xy)),
        box_muller_12(1.0/float(0xffffffffu) * float2(pcg3d(id.xyz + uint3(0, 0, 4)).xy)));
    float2 final_spectrum = noise * spectrum;

    // We do not want to sample the oceanographic spectrum multiple times at the same wavenumber.
    // Doing so means we are oversampling and it will produce wrong results.
    bool cutoff = false;
    if ((wavenumber < pars.spectral_cutoffs_low[id.z]) || (wavenumber > pars.spectral_cutoffs_high[id.z]))
    {
        cutoff = true;
    }

    // We cannot sample all wavelengths without loss of information according to the Shannon-Nyquist sampling theorem,
    // so we need to cut off the spectrum at those wavelengths as well.
    // This is not necessarily handled by the cutoffs provided so we may ensure it.
    bool sampling_limit = false;
    float k_min = sqrt(2.0) * 2.0 * MC_PI / pars.lengthscales[id.z];
    float k_max = MC_PI * pars.size / pars.lengthscales[id.z];
    if((wavenumber <= k_min + 0.001) || (wavenumber >= k_max - 0.001))
    {
        sampling_limit = true;
    }

    if ( cutoff || sampling_limit )
    {
        final_spectrum = 0.0;
        omega = 0.0;
    }

    bnd.spectrum_tex.store_2d_array(id.xyz, float4(final_spectrum, k));
    bnd.angular_frequency_tex.store_2d_array(id.xyz, omega);
}
