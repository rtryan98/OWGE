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

[numthreads(32, 32, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    Bindset bnd = read_bindset_uniform<Bindset>(pc.bindset_buffer, pc.bindset_offset);

    float4 spectrum_wavenumber = bnd.spectrum_tex.load_2d_array<float4>(id.xyz);
    float2 spectrum = spectrum_wavenumber.xy;
    float2 wavenumber = spectrum_wavenumber.zw;
    float mag = max(0.0001, length(wavenumber));
    uint2 conjugate_pos = (uint2(bnd.size, bnd.size) - id.xy) % bnd.size;
    float2 spectrum_minus_k = complex_conjugate(bnd.spectrum_tex.load_2d_array<float4>(uint3(conjugate_pos, id.z)).xy);
    float omega_k = bnd.angular_frequency_tex.load_2d_array<float>(id.xyz);

    float phi = bnd.time * omega_k;
    float2 cmul_term = complex_from_polar(1.0, phi);
    float2 developed_spectrum = 0.5 * (complex_mul(spectrum, cmul_term) + complex_mul(spectrum_minus_k, complex_conjugate(cmul_term)));

    float2 rotated_developed_spectrum = float2(-developed_spectrum.y, developed_spectrum.x); // multiplication by i

    float2 displacement_x = rotated_developed_spectrum * wavenumber.x * (1.0 / mag);
    float2 displacement_y = rotated_developed_spectrum * wavenumber.y * (1.0 / mag);
    float2 displacement_z = developed_spectrum;

    float2 displacement_x_dx = -developed_spectrum * wavenumber.x * wavenumber.x * (1.0 / mag);
    float2 displacement_y_dx = -developed_spectrum * wavenumber.y * wavenumber.x * (1.0 / mag);
    float2 displacement_z_dx = rotated_developed_spectrum * wavenumber.x;

    // displacement_x_dy is equal to displacement_y_dx
    float2 displacement_y_dy = -developed_spectrum * wavenumber.y * wavenumber.y * (1.0 / mag);
    float2 displacement_z_dy = rotated_developed_spectrum * wavenumber.y;

    // We have hermitian symmetric spectra, so we can calculate two IFFTs in one.
    // F^{-1}[F[a] + i*F[b]]
    float2 packed_spectrum_x_y = displacement_x + float2(-displacement_y.y, displacement_y.x);
    float2 packed_spectrum_z_x_dx = displacement_z + float2(-displacement_x_dx.y, displacement_x_dx.x);
    float2 packed_spectrum_y_dx_z_dx = displacement_y_dx + float2(-displacement_z_dx.y, displacement_z_dx.x);
    float2 packed_spectrum_y_dy_z_dy = displacement_y_dy + float2(-displacement_z_dy.y, displacement_z_dy.x);

    uint2 shifted_pos = (id.xy + uint2(bnd.size, bnd.size) / 2) % bnd.size;

    bnd.packed_spectrum_x_y.store_2d_array<float2>(uint3(shifted_pos, id.z), packed_spectrum_x_y);
    bnd.packed_spectrum_z_x_dx.store_2d_array<float2>(uint3(shifted_pos, id.z), packed_spectrum_z_x_dx);
    bnd.packed_spectrum_y_dx_z_dx.store_2d_array<float2>(uint3(shifted_pos, id.z), packed_spectrum_y_dx_z_dx);
    bnd.packed_spectrum_y_dy_z_dy.store_2d_array<float2>(uint3(shifted_pos, id.z), packed_spectrum_y_dy_z_dy);
}
