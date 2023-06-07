#ifndef OWGE_OCEANOGRAPHY
#define OWGE_OCEANOGRAPHY

#include "owge_shaders/math.hlsli"
#include "owge_shaders/math_constants.hlsli"

// Dispersion relationships

float oceanography_dispersion_deep(float k, float g)
{
    return sqrt(g * k);
}

float oceanography_dispersion_deep_d_dk(float k, float g)
{
    return g / (2.0f * sqrt(g * k));
}

float oceanography_dispersion_finite_depth(float k, float g, float h)
{
    return sqrt(g * k * tanh(k * h));
}

float oceanography_dispersion_finite_depth_d_dk(float k, float g, float h)
{
    return g * (tanh(k * h) + k * h * pow(math_sech(k * h), 2.0f)) / (2.0f * sqrt(g * k * tanh(k * h)));
}

float oceanography_dispersion_capillary(float k, float g, float h)
{
    static const float sigma = 0.074f;
    static const float rho = 1000.0f;
    static const float sigma_over_rho = sigma / rho;
    return sqrt((g * k + (sigma_over_rho) * pow(k, 3.0f)) * tanh(k * h));
}

float oceanography_dispersion_capillary_d_dk(float k, float g, float h)
{
    static const float sigma = 0.074f;
    static const float rho = 1000.0f;
    static const float sigma_over_rho = sigma / rho;

    float k2 = k * k;
    float k3 = k2 * k;

    float a = (3.0f * sigma_over_rho * k2 + g) * tanh(k * h);
    float b = h * (sigma_over_rho * k3 + g * k) * pow(math_sech(k * h), 2.0f);
    float c = 2.0f * sqrt((sigma_over_rho * k3 + g * k) * tanh(k * h));

    return (a + b) / c;
}


// Non-directional wave spectra

float oceanography_phillips_spectrum(float omega, float alpha, float g)
{
    return alpha * 2.0f * mc_pi * (pow(g, 2.0f) / pow(omega, 5.0f));
}

float oceanography_pierson_moskowith_omega_0(float g, float u)
{
    return g / (1.026f * u);
}

float oceanography_pierson_moskowith_omega_peak(float g, float u)
{
    return (0.855f * g) / u;
}

float oceanography_pierson_moskowith_spectrum(float omega, float omega_peak, float g, float u)
{
    float alpha = 0.0081f;
    float beta = 0.74f;
    return ((alpha * pow(g, 2.0f))/pow(omega, 5.0f)) * exp(-beta * pow(omega_peak / omega, 4.0f));
}

float oceanography_generalized_a_b_spectrum(float omega, float a, float b)
{
    return (a / pow(omega, 5.0f)) * exp((-b) / pow(omega, 4.0f));
}

float oceanography_jonswap_omega_peak(float g, float u, float f)
{
    return 22.0f * (pow(g, 2.0f) / (u * f));
}

float oceanography_jonswap_spectrum(float omega, float omega_peak, float u, float g, float f)
{
    float gamma = 3.3f;
    float sigma_0 = 0.07f;
    float sigma_1 = 0.09f;
    float sigma = omega <= omega_peak
        ? sigma_0
        : sigma_1;
    float alpha = 0.076f * pow(pow(u, 2.0f) / (f * g), 0.22f);
    float r = exp(-(pow(omega - omega_peak, 2.0f) / (2.0f * pow(sigma, 2.0f) * pow(omega_peak, 2.0f))));
    return ((alpha * pow(g, 2.0f))/(pow(omega, 5.0f))) * exp(-1.25f * pow(omega_peak / omega, 4.0f)) * pow(gamma, 4.0f);
}

float oceanography_tma_spectrum(float omega, float omega_peak, float u, float g, float f, float h)
{
    float omega_h = omega * sqrt(h / g);
    float phi_0 = 0.5f * pow(omega_h, 2.0f);
    float phi_1 = 1.0f - 0.5f * pow(2.0f - omega_h, 2.0f);
    float phi = omega_h <= 1.0f
        ? phi_0
        : phi_1;
    return phi * oceanography_jonswap_spectrum(omega, omega_peak, u, g, f);
}

// omega_m saturates the floating point range and should be precalculated on CPU.
// It is calculated by 0.61826 + 0.0000003529 * f - 0.00197508 * sqrt(f) + (62.554 / sqrt(f)) - (290.2 / f)
float oceanography_v_yu_karaev_spectrum(float omega, float omega_peak, float omega_m, float u, float g, float f)
{
    static const float omega_gc = 64.0;
    static const float omega_c = 298.0;
    float alpha_m = 0.3713 + 0.29024 * u + (0.2902 / u);

    float alpha_1 = oceanography_jonswap_spectrum(omega, omega_peak, u, g, f);
    float alpha_2 = oceanography_jonswap_spectrum(1.2 * omega_m, omega_peak, u, g, f) * pow(1.2 * omega_m, 4.0);
    float alpha_3 = alpha_2 * alpha_m * omega_m;
    float alpha_4 = alpha_3 / pow(omega_gc, 2.3);
    // float alpha_5 = alpha_3; // == alpha_4 * pow(omega_gc, 2.3)

    return omega <= 1.2 * omega_m
        ? alpha_1
        : omega <= alpha_m * omega_m
            ? alpha_2 / pow(omega, 4.0)
            : (omega <= omega_gc || omega > omega_c)
                ? alpha_3 / pow(omega, 5.0)
                : alpha_4 / pow(omega, 2.7);
}

// Directional wave spectra

float oceanography_positive_cos_sq_directional_spreading(float theta)
{
    float a = 2.0f / mc_pi * pow(cos(theta), 2.0f);
    bool condition = -mc_pi / 2.0f < theta && theta < mc_pi / 2.0f;
    return condition
        ? a
        : 0.0f;
}

float __oceanography_mitsuyasu_s(float omega, float omega_peak, float u, float g)
{
    float s_p = 11.5f * pow((omega_peak * u) / g, -2.5f);
    float s0 = pow(s_p,  5.0f);
    float s1 = pow(s_p, -2.5f);
    return omega > omega_peak
        ? s1
        : s0;
}

float __oceanography_mitsuyasu_q(float s)
{
    float a = pow(2.0f, 2.0f * s - 1) / mc_pi;
    float b = pow(math_stirling_approximation(s + 1), 2.0f);
    float c = math_stirling_approximation(2.0f * s + 1);
    return a * (b / c);
}

float oceanography_mitsuyasu_directional_spreading(float omega, float omega_peak, float theta, float u, float g)
{
    float s = __oceanography_mitsuyasu_s(omega, omega_peak, u, g);
    float q_s = __oceanography_mitsuyasu_q(s);
    return q_s * pow(abs(cos(theta / 2.0f)), 2.0f);
}

float __oceanography_hasselmann_s(float omega, float omega_peak, float u, float g)
{
    float s0 = 6.97f * pow(omega / omega_peak, 4.06f);
    float s1_exp = -2.33f - 1.45f * (((u * omega_peak) / g) - 1.17f);
    float s1 = 9.77f * pow(omega / omega_peak, s1_exp);
    return omega > omega_peak
        ? s1
        : s0;
}

float oceanography_hasselmann_directional_spreading(float omega, float omega_peak, float theta, float u, float g)
{
    float s = __oceanography_hasselmann_s(omega, omega_peak, u, g);
    float q_s = __oceanography_mitsuyasu_q(s);
    return q_s * pow(abs(cos(theta / 2.0f)), 2.0f);
}

float __oceanography_donelan_banner_beta_s(float omega, float omega_peak)
{
    float om_over_omp = omega / omega_peak;
    float epsilon = -0.4f + 0.8393 * exp(-0.567f * pow(log(om_over_omp), 2.0f));
    float beta_s_0 = 2.61f * pow(om_over_omp, 1.3f);
    float beta_s_1 = 2.28f * pow(om_over_omp, -1.3f);
    float beta_s_2 = pow(10.0f, epsilon);
    return om_over_omp < 0.95f
        ? beta_s_0
        : om_over_omp < 1.6f
            ? beta_s_1
            : beta_s_2;
    // Clamping om_over_omp between 0.54 and 0.95 is correct according to [Donelan et al. 1985],
    // however, cutting off om_over_omp at 0.54 and returning 0.0 instead
    // will result in a less pleasing result according to [Horvath 2015].
    // TODO: Maybe parameterize this cutoff?
}

float oceanography_donelan_banner_directional_spreading(float omega, float omega_peak, float theta)
{
    float beta_s = __oceanography_donelan_banner_beta_s(omega, omega_peak);
    return (beta_s/(2.0f * tanh(beta_s * mc_pi))) * pow(math_sech(beta_s * theta), 2.0f);
}

float oceanography_flat_directional_spreading()
{
    return 1.0f / (2.0f * mc_pi);
}

float oceanography_mixed_directional_spreading(float dir_spread_a, float dir_spread_b, float tau)
{
    return lerp(dir_spread_a, dir_spread_b, tau);
}

#endif // OWGE_OCEANOGRAPHY
