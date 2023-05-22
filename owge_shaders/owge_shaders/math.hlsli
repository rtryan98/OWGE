#ifndef OWGE_MATH
#define OWGE_MATH

#include "owge_shaders/math_constants.hlsli"

float math_sech(float x)
{
    return 1.0f / cosh(x);
}

float math_csch(float x)
{
    return 1.0f / sinh(x);
}

float math_stirling_approximation(float n)
{
    return sqrt(2.0f * mc_pi * n) * pow(n / mc_e, n);
}

#endif // OWGE_MATH
