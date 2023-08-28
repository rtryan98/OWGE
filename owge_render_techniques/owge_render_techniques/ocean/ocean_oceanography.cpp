#include "owge_render_techniques/ocean/ocean_oceanography.hpp"

#include <DirectXMath.h>

namespace owge
{
constexpr std::pair<float, float> OCEAN_SAMPLE_REJECT = { -1.0f, -1.0f };

std::pair<float, float> ocean_calculate_dispersion(const Ocean_Settings& settings, float k)
{
    constexpr static float SIGMA_OVER_RHO = 0.074f / 1000.0f;

    float dispersion = sqrtf((settings.gravity * k + SIGMA_OVER_RHO * powf(k, 3.0f)) * tanhf(settings.ocean_depth * k));
    float dispersion_derivative = ((3.0f * SIGMA_OVER_RHO * powf(k, 2.0f) + settings.gravity) + (settings.ocean_depth * (SIGMA_OVER_RHO * powf(k, 3.0f) + settings.gravity * k) * powf(1.0f / coshf(k * settings.ocean_depth), 2.0f))) / (2.0f * dispersion);
    return { dispersion, dispersion_derivative };
}

float ocean_calculate_jonswap_spectrum(const Ocean_Settings& settings, float omega, bool local)
{
    float f = local
        ? settings.local_spectrum.fetch
        : settings.swell_spectrum.fetch;
    float u = local
        ? settings.local_spectrum.wind_speed
        : settings.swell_spectrum.wind_speed;
    float omega_peak = 22.0f * (powf(settings.gravity, 2.0f) / (u * f));
    float gamma = 3.3f;
    float sigma_0 = 0.07f;
    float sigma_1 = 0.09f;
    float sigma = omega <= omega_peak
        ? sigma_0
        : sigma_1;
    float alpha = 0.076f * powf(powf(u, 2.0f) / (f * settings.gravity), 0.22f);
    float r = expf(-(powf(omega - omega_peak, 2.0f) / (2.0f * powf(sigma, 2.0f) * powf(omega_peak, 2.0f))));
    return ((alpha * powf(settings.gravity, 2.0f)) / (powf(omega, 5.0f))) * expf(-1.25f * powf(omega_peak / omega, 4.0f)) * powf(gamma, r);
}

float ocean_calculate_tma_spectrum(const Ocean_Settings& settings, float omega)
{
    float omega_h = omega * sqrtf(settings.ocean_depth / settings.gravity);
    float phi_0 = 0.5f * powf(omega_h, 2.0f);
    float phi_1 = 1.0f - 0.5f * powf(2.0f - omega_h, 2.0f);
    float phi = omega_h <= 1.0f
        ? phi_0
        : omega_h <= 2.0f
        ? phi_1
        : 1.0f;
    return ocean_calculate_jonswap_spectrum(settings, omega, true) * phi;
}

float ocean_calculate_nondirectional_spectrum(const Ocean_Settings& settings, float omega)
{
    return ocean_calculate_tma_spectrum(settings, omega);
}

std::pair<float, float> ocean_calculate_spectrum_sample_for_cascade(
    const Ocean_Settings& settings, uint32_t cascade, uint32_t sample, bool allow_reject)
{
    using namespace DirectX;

    if (sample == 0u)
    {
        if (allow_reject)
        {
            return OCEAN_SAMPLE_REJECT;
        }
        return { 0.0f, 0.0f };
    }
    float delta_k = (2.0f * XM_PI) / float(settings.length_scales[cascade]);
    float k = sample * delta_k;

    auto dispersion = ocean_calculate_dispersion(settings, k);
    float omega = dispersion.first;
    float omega_d_dk = dispersion.second;

    if (cascade > 0u)
    {
        if (k > (settings.size / 2.0f) * delta_k)
        {
            if (allow_reject)
            {
                return OCEAN_SAMPLE_REJECT;
            }
        }
        if (k > (2.0f * XM_PI) / settings.length_scales[cascade - 1])
        {
            if (allow_reject)
            {
                return OCEAN_SAMPLE_REJECT;
            }
        }
    }

    float spectrum = sqrtf(2.0f * ocean_calculate_nondirectional_spectrum(settings, omega) * (omega_d_dk / k) * delta_k * delta_k);

    return { spectrum, omega };
}

std::pair<std::vector<float>, std::vector<float>> ocean_calculate_spectrum_for_cascade(const Ocean_Settings& settings, uint32_t cascade)
{
    std::vector<float> spectrum_values = {};
    std::vector<float> wavenumbers = {};
    spectrum_values.reserve(settings.size / 2 - 1);
    wavenumbers.reserve(settings.size / 2 - 1);
    for (uint32_t i = 1; i < settings.size / 2; ++i)
    {
        auto sample = ocean_calculate_spectrum_sample_for_cascade(settings, cascade, i, false);
        if (sample.first == -1.0f)
        {
            continue;
        }
        spectrum_values.push_back(sample.first);
        wavenumbers.push_back(sample.second);
    }
    return { spectrum_values, wavenumbers };
}
}
