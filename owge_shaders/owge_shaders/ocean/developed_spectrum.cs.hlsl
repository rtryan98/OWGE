#include "owge_shaders/bindless.hlsli"
#include "owge_shaders/complex.hlsli"

struct Bindset
{
    RW_Texture spectrum_tex;
    RW_Texture angular_frequency_tex;
    float time;
    uint size;

    RW_Texture packed_spectrum_x_y;
    RW_Texture packed_spectrum_z_x_dx;
    RW_Texture packed_spectrum_y_dx_z_dx;
    RW_Texture packed_spectrum_y_dy_z_dy;
};

struct Push_Constants
{
    uint bindset_buffer;
    uint bindset_offset;
    uint __pad0;
    uint __pad1;
};
ConstantBuffer<Push_Constants> pc : register(b0, space0);

float2 mul_i(float2 complex)
{
    return float2(-complex.y, complex.x);
}

[numthreads(32, 32, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    Bindset bnd = read_bindset_uniform<Bindset>(pc.bindset_buffer, pc.bindset_offset);

    float4 spectrum_and_k = bnd.spectrum_tex.load_2d_array<float4>(id.xyz);
    float2 spectrum = spectrum_and_k.xy;
    float2 k = spectrum_and_k.zw;
    float one_over_k_len = 1.0 / max(0.001, length(k));
    float2 spectrum_minus_k = complex_conjugate(bnd.spectrum_tex.load_2d_array<float4>(uint3((bnd.size - id.x) % bnd.size, (bnd.size - id.y) % bnd.size, id.z)).xy);
    float omega_k = bnd.angular_frequency_tex.load_2d_array<float>(id.xyz);

    float2 cmul_term = complex_from_polar(1.0, bnd.time * omega_k);
    float2 h = 0.5 * (complex_mul(spectrum, cmul_term) + complex_mul(spectrum_minus_k, complex_conjugate(cmul_term)));
    float2 ih = mul_i(h);

    float2 displacement_x = ih * k.x * one_over_k_len;
    float2 displacement_y = ih * k.y * one_over_k_len;
    float2 displacement_z = h;

    float2 displacement_x_dx = -h * k.x * k.x * one_over_k_len;
    float2 displacement_y_dx = -h * k.y * k.x * one_over_k_len;
    float2 displacement_z_dx = ih * k.x;

    // displacement_x_dy is equal to displacement_y_dx
    float2 displacement_y_dy = -h * k.y * k.y * one_over_k_len;
    float2 displacement_z_dy = ih * k.y;

    // We have hermitian symmetric spectra, so we can calculate two IFFTs in one.
    // F^{-1}[F[a] + i*F[b]]
    float2 packed_spectrum_x_y = displacement_x + mul_i(displacement_y);
    float2 packed_spectrum_z_x_dx = displacement_z + mul_i(displacement_x_dx);
    float2 packed_spectrum_y_dx_z_dx = displacement_y_dx + mul_i(displacement_z_dx);
    float2 packed_spectrum_y_dy_z_dy = displacement_y_dy + mul_i(displacement_z_dy);

    uint2 shifted_pos = (id.xy + uint2(bnd.size, bnd.size) / 2) % bnd.size;

    bnd.packed_spectrum_x_y.store_2d_array<float2>(uint3(shifted_pos, id.z), packed_spectrum_x_y);
    bnd.packed_spectrum_z_x_dx.store_2d_array<float2>(uint3(shifted_pos, id.z), packed_spectrum_z_x_dx);
    bnd.packed_spectrum_y_dx_z_dx.store_2d_array<float2>(uint3(shifted_pos, id.z), packed_spectrum_y_dx_z_dx);
    bnd.packed_spectrum_y_dy_z_dy.store_2d_array<float2>(uint3(shifted_pos, id.z), packed_spectrum_y_dy_z_dy);
}
