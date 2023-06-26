#pragma once

#include "owge_render_engine/render_procedure/render_procedure.hpp"
#include "owge_render_engine/resource.hpp"
#include "owge_render_engine/bindless.hpp"

#include <vector>

namespace owge
{

struct Ocean_Spectrum_Spectra_Parameters
{
    float wind_speed;
    float fetch;
    float v_yu_karaev_spectrum_omega_m;
};

float ocean_spectrum_calculate_v_yu_karaev_spectrum_omega_m(float fetch);

struct Ocean_Spectrum_Ocean_Parameters
{
    uint32_t size;
    float length_scale;
    float gravity;
    float ocean_depth;
    Ocean_Spectrum_Spectra_Parameters spectra[2];
};

struct Ocean_Settings
{

};

class Ocean_Calculate_Spectra_Render_Procedure : public Render_Procedure
{
public:
    Ocean_Calculate_Spectra_Render_Procedure(
        Render_Engine* render_engine,
        const Ocean_Spectrum_Ocean_Parameters& ocean_spectrum_pars);
    ~Ocean_Calculate_Spectra_Render_Procedure();

    virtual void process(const Render_Procedure_Payload& payload) override;

private:
    Render_Engine* m_render_engine;

    Shader_Handle m_initial_spectrum_shader;
    Pipeline_Handle m_initial_spectrum_compute_pso;
    Shader_Handle m_developed_spectrum_shader;
    Pipeline_Handle m_developed_spectrum_compute_pso;
    Texture_Handle m_initial_spectrum_texture;
    Texture_Handle m_angular_frequency_texture;
    Texture_Handle m_developed_spectrum_texture;
    Buffer_Handle m_initial_spectrum_ocean_params_buffer;

    Bindset m_initial_spectrum_bindset;
    Bindset m_developed_spectrum_bindset;

    Ocean_Spectrum_Ocean_Parameters m_ocean_spectrum_pars;
    bool m_is_dirty = true;
    float m_time = 0.0f;
};

struct Ocean_Tile_Settings
{

};

class Ocean_Tile_Render_Procedure : public Render_Procedure
{
public:
    Ocean_Tile_Render_Procedure();
    virtual void process(const Render_Procedure_Payload& payload) override;
};
}
