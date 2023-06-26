#ifndef OWGE_RAND
#define OWGE_RAND

#include "owge_shaders/math_constants.hlsli"

// https://stackoverflow.com/questions/4200224/random-noise-functions-for-glsl
float rand(float2 co)
{
    return frac(sin(dot(co, float2(12.9898, 78.233))) * 43758.5453);
}

// Hash without sine, Copyright (c) 2014 David Hoskins, MIT license.
// https://www.shadertoy.com/view/4djSRW
float hash_nosine_11(float p)
{
    p = frac(p * 0.1031);
    p *= p + 33.33;
    p *= p + p;
    return frac(p);
}

float hash_nosine_12(float2 p)
{
	float3 p3 = frac(float3(p.xyx) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.x + p3.y) * p3.z);
}

float hash_nosine_13(float3 p3)
{
	p3  = frac(p3 * 0.1031);
    p3 += dot(p3, p3.zyx + 31.32);
    return frac((p3.x + p3.y) * p3.z);
}

float hash_nosine_14(float4 p4)
{
	p4 = frac(p4 * float4(0.1031, 0.1030, 0.0973, 0.1099));
    p4 += dot(p4, p4.wzxy+33.33);
    return frac((p4.x + p4.y) * (p4.z + p4.w));
}

float2 hash_nosine_21(float p)
{
	float3 p3 = frac(float3(p, p, p) * float3(0.1031, 0.1030, 0.0973));
	p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xx+p3.yz) * p3.zy);
}

float2 hash_nosine_22(float2 p)
{
	float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xx + p3.yz) * p3.zy);
}

float2 hash_nosine_23(float3 p3)
{
	p3 = frac(p3 * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return frac((p3.xx+p3.yz) * p3.zy);
}

float3 hash_nosine_31(float p)
{
   float3 p3 = frac(float3(p, p, p) * float3(0.1031, 0.1030, 0.0973));
   p3 += dot(p3, p3.yzx + 33.33);
   return frac((p3.xxy+p3.yzz) * p3.zyx); 
}

float3 hash_nosine_32(float2 p)
{
	float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return frac((p3.xxy + p3.yzz) * p3.zyx);
}

float3 hash_nosine_33(float3 p3)
{
	p3 = frac(p3 * float3(0.1031, 0.1030, 0.0973));
    p3 += dot(p3, p3.yxz + 33.33);
    return frac((p3.xxy + p3.yxx) * p3.zyx);
}

float4 hash_nosine_41(float p)
{
	float4 p4 = frac(float4(p, p, p, p) * float4(0.1031, 0.1030, 0.0973, 0.1099));
    p4 += dot(p4, p4.wzxy + 33.33);
    return frac((p4.xxyz + p4.yzzw) * p4.zywx);
}

float4 hash_nosine_42(float2 p)
{
	float4 p4 = frac(float4(p.xyxy) * float4(0.1031, 0.1030, 0.0973, 0.1099));
    p4 += dot(p4, p4.wzxy + 33.33);
    return frac((p4.xxyz + p4.yzzw) * p4.zywx);
}

float4 hash_nosine_43(float3 p)
{
	float4 p4 = frac(float4(p.xyzx) * float4(0.1031, 0.1030, 0.0973, 0.1099));
    p4 += dot(p4, p4.wzxy + 33.33);
    return frac((p4.xxyz + p4.yzzw) * p4.zywx);
}

float4 hash_nosine_44(float4 p4)
{
	p4 = frac(p4 * float4(0.1031, 0.1030, 0.0973, 0.1099));
    p4 += dot(p4, p4.wzxy + 33.33);
    return frac((p4.xxyz + p4.yzzw) *p4.zywx);
}

float2 box_muller_22(float2 uniform_distr_vals, float mean, float std_deviation)
{
    static const float epsilon = 0.00001;

    float u1 = -2.0 * log(clamp(uniform_distr_vals.x, epsilon, 1.0));
    float u2 = uniform_distr_vals.y;
    float mag = std_deviation * sqrt(u1);
    return mag * float2(cos(2.0 * mc_pi * u2) + std_deviation, sin(2.0 * mc_pi * u2) + std_deviation);
}

#endif