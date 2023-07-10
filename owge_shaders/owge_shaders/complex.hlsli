#ifndef OWGE_COMPLEX
#define OWGE_COMPLEX

float2 complex_from_polar(float r, float phi)
{
    float2 result;
    sincos(phi, result.y, result.x);
    return r * result;
}

float2 complex_add(float2 a, float2 b)
{
    return a + b;
}

float2 complex_sub(float2 a, float2 b)
{
    return a - b;
}

float2 complex_mul(float2 a, float2 b)
{
    return float2(a.x * b.x - a.y * b.y, a.y * b.x + a.x * b.y);
}

float2 complex_div(float2 a, float2 b)
{
    float real = a.x * b.x + a.y * b.y;
    float imaginary = a.y * b.x - a.x * b.y;
    float divisor = b.x * b.x + b.y * b.y;
    return float2((real / divisor),(imaginary / divisor));
}

float2 complex_conjugate(float2 a)
{
    return float2(a.x, -a.y);
}

#endif // OWGE_COMPLEX
