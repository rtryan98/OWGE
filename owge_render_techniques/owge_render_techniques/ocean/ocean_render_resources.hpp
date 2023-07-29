#pragma once

#include <owge_render_engine/resource.hpp>
#include <owge_render_engine/bindless.hpp>

#include <DirectXMath.h>

namespace owge
{
using namespace DirectX;

class Render_Engine;
struct Ocean_Settings;

struct Ocean_Simulation_Spectra
{
    float wind_speed;
    float fetch;
    float v_yu_karaev_spectrum_omega_m;
};

struct Ocean_Simulation_Initial_Spectrum_Parameter_Buffer
{
    uint32_t size;
    float length_scale;
    float gravity;
    float ocean_depth;
    Ocean_Simulation_Spectra spectra[2];
};

struct Ocean_Initial_Spectrum_Shader_Bindset
{
    uint32_t ocean_params_buf_idx;
    uint32_t initial_spectrum_tex_idx;
    uint32_t angular_frequency_tex_idx;
};

struct Ocean_Developed_Spectrum_Shader_Bindset
{
    uint32_t initial_spectrum_tex_idx;
    uint32_t angular_frequency_tex_idx;
    uint32_t developed_spectrum_tex_idx;
    float time;
    uint32_t size;
};

struct Ocean_FFT_Constants
{
    uint32_t texture;
    uint32_t vertical;
    uint32_t inverse;
};

struct Ocean_Surface_VS_Render_Data
{
    XMFLOAT2 pos;
};

struct Ocean_Surface_VS_Bindset
{
    uint32_t vertex_buffer;
    uint32_t render_data;
};

struct Ocean_Surface_PS_Bindset
{

};

struct Ocean_Simulation_Render_Resources
{
    void create(
        Render_Engine* render_engine,
        Ocean_Settings* settings);
    void destroy(
        Render_Engine* render_engine);
    void resize_textures(
        Render_Engine* render_engine,
        Ocean_Settings* settings);

    Shader_Handle fft_128_shader;
    Shader_Handle fft_256_shader;
    Shader_Handle fft_512_shader;
    Shader_Handle initial_spectrum_shader;
    Shader_Handle developed_spectrum_shader;
    Pipeline_Handle initial_spectrum_pso;
    Pipeline_Handle developed_spectrum_pso;
    Pipeline_Handle fft_128_pso;
    Pipeline_Handle fft_256_pso;
    Pipeline_Handle fft_512_pso;

    Buffer_Handle initial_spectrum_ocean_params_buffer;
    Texture_Handle initial_spectrum_texture;
    Texture_Handle angular_frequency_texture;
    Texture_Handle developed_spectrum_texture;

    Bindset initial_spectrum_bindset;
    Bindset developed_spectrum_bindset;

    Texture_Handle displacement_x_y_z_texture;
    Texture_Handle normals_x_y_z_texture;

    Buffer_Handle ocean_surface_vertex_buffer;
    Buffer_Handle ocean_surface_index_buffer;
    Buffer_Handle ocean_surface_vs_render_data;

    Shader_Handle surface_plane_vs;
    Shader_Handle surface_plane_ps;
    Pipeline_Handle surface_plane_pso;

    Bindset surface_render_vs_bindset;
    Bindset surface_render_ps_bindset;
};
}
