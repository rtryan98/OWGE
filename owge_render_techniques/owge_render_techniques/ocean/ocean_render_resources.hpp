#pragma once

#include "owge_render_techniques/ocean/ocean_settings.hpp"

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
    float length_scales[4];
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
    float time;
    uint32_t size;

    uint32_t packed_spectrum_x_y_tex_idx;
    uint32_t packed_spectrum_z_x_dx_tex_idx;
    uint32_t packed_spectrum_y_dx_z_dx_tex_idx;
    uint32_t packed_spectrum_y_dy_z_dy_tex_idx;
};

struct Ocean_FFT_Constants
{
    uint32_t texture;
    uint32_t vertical;
    uint32_t inverse;
};

struct Ocean_Texture_Reorder_Shader_Bindset
{
    uint32_t packed_x_y;
    uint32_t packed_z_x_dx;
    uint32_t packed_y_dx_z_dx;
    uint32_t packed_y_dy_z_dy;

    uint32_t displacement;
    uint32_t derivatives;
    uint32_t folding_map;
};

struct Ocean_Surface_Bindset
{
    uint32_t vertex_buffer;
    uint32_t render_data;
    uint32_t displacement_texture;
    uint32_t derivatives_texture;
    uint32_t jacobian_texture;
    uint32_t surface_sampler;
};

struct Ocean_Surface_VS_Render_Data
{
    XMFLOAT4X4 view_proj;
    float length_scales[Ocean_Settings::MAX_CASCADES];
    XMFLOAT4 camera_position;
    float scale;
    float offset;
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

    Bindset texture_reorder_bindset;
    Shader_Handle texture_reorder_shader;
    Pipeline_Handle texture_reorder_pso;
    Texture_Handle packed_x_y_texture;
    Texture_Handle packed_z_x_dx_texture;
    Texture_Handle packed_y_dx_z_dx_texture;
    Texture_Handle packed_y_dy_z_dy_texture;

    Bindset initial_spectrum_bindset;
    Bindset developed_spectrum_bindset;

    Texture_Handle displacement_x_y_z_texture;
    Texture_Handle derivatives_texture;
    Texture_Handle jacobian_texture;

    Buffer_Handle ocean_surface_vertex_buffer;
    Buffer_Handle ocean_surface_index_buffer;
    Buffer_Handle ocean_surface_vs_render_data_buffer;
    uint32_t ocean_surface_index_count;
    Sampler_Handle ocean_surface_sampler;

    Shader_Handle surface_plane_vs;
    Shader_Handle surface_plane_ps;
    Pipeline_Handle surface_plane_pso;

    Bindset surface_render_vs_bindset;
    Bindset surface_render_ps_bindset;

private:
    void create_simulation_shaders(Render_Engine* render_engine);
    void destroy_simulation_shaders(Render_Engine* render_engine);

    void create_simulation_resources(Render_Engine* render_engine);
    void destroy_simulation_resources(Render_Engine* render_engine);

    void create_surface_shaders(Render_Engine* render_engine);
    void destroy_surface_shaders(Render_Engine* render_engine);

    void create_surface_resources(Render_Engine* render_engine);
    void destroy_surface_resources(Render_Engine* render_engine);

    void update_persistent_bindsets(Render_Engine* render_engine);
};
}
