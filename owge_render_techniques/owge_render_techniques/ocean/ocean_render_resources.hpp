#pragma once

#include <owge_render_engine/resource.hpp>
#include <owge_render_engine/bindless.hpp>

namespace owge
{
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
};
}
