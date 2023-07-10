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

float math_kronecker_delta(uint2 ij)
{
    return ij.x == ij.y
        ? 1.0
        : 0.0;
}

float math_stirling_approximation(float n)
{
    return sqrt(2.0f * MC_PI * n) * pow(n / MC_E, n);
}

#endif // OWGE_MATH
