#ifndef OWGE_FFT_SIZE
#error "Required define OWGE_FFT_SIZE was not set."
#endif
#ifndef OWGE_FFT_LOG_SIZE
#error "Required define OWGE_FFT_LOG_SIZE was not set."
#endif

#if !(OWGE_FFT_SIZE == 512 || OWGE_FFT_SIZE == 256 || OWGE_FFT_SIZE == 128)
#error "Invalid value for OWGE_FFT_SIZE."
#endif

#include "owge_shaders/bindless.hlsli"
#include "owge_shaders/complex.hlsli"
#include "owge_shaders/math_constants.hlsli"

groupshared float2 ping_pong_buffer[2][OWGE_FFT_SIZE];

struct Push_Constants
{
    RW_Texture input_output;
    uint vertical; // Horizontal if 0, Vertical if 1
    uint inverse;  // Forward if 0, Inverse if 1
    uint __pad0;
};
ConstantBuffer<Push_Constants> pc : register(b0, space0);

void butterfly(uint thread_idx, uint iteration, out uint2 twiddle_indices, out float2 twiddle_factor)
{
    uint butterfly_size = 2u << iteration;
    uint butterfly_half_size = (butterfly_size >> 1u);
    uint butterfly_size_relative_idx = thread_idx % butterfly_size;
    uint butterfly_start_idx = butterfly_size * (thread_idx / butterfly_size);

    uint base_idx = butterfly_start_idx + (butterfly_size_relative_idx % butterfly_half_size);
    uint lower_idx = base_idx;
    uint upper_idx = base_idx + butterfly_half_size;
    twiddle_indices = uint2(lower_idx, upper_idx);

    float arg = -2.0 * MC_PI * float(butterfly_size_relative_idx) / float(butterfly_size);
    sincos(arg, twiddle_factor.y, twiddle_factor.x);
    if (pc.inverse)
    {
        twiddle_factor.y = -twiddle_factor.y;
    }
}

// Cooley-Tukey FFT
[numthreads(OWGE_FFT_SIZE, 1, 1)]
void cs_main(uint3 id : SV_DispatchThreadID)
{
    bool ping_pong = false;
    uint2 texpos = bool(pc.vertical)
        ? id.xy
        : id.yx;
    ping_pong_buffer[ping_pong][reversebits(id.x) >> (32 - OWGE_FFT_LOG_SIZE)] = pc.input_output.load_2d_array<float2>(uint3(texpos, id.z));
    GroupMemoryBarrierWithGroupSync();

    [unroll(OWGE_FFT_LOG_SIZE)] for (uint i = 0; i < OWGE_FFT_LOG_SIZE; i++)
    {
        uint2 twiddle_indices;
        float2 twiddle_factor;
        butterfly(id.x, i, twiddle_indices, twiddle_factor);
        ping_pong_buffer[!ping_pong][id.x] =
            ping_pong_buffer[ping_pong][twiddle_indices.x] + complex_mul(ping_pong_buffer[ping_pong][twiddle_indices.y], twiddle_factor);
        GroupMemoryBarrierWithGroupSync();
        ping_pong = !ping_pong;
    }

    pc.input_output.store_2d_array(uint3(texpos, id.z), ping_pong_buffer[ping_pong][id.x]);
}
