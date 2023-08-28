#pragma once

#include "owge_render_techniques/render_technique_settings.hpp"

#include <cstdint>
#include <array>
#include <vector>

namespace owge
{
enum class Ocean_Spectrum : uint32_t
{
    Phillips = 0,
    Pierson_Moskowitz,
    Generalized_A_B,
    JONSWAP,
    TMA,
    V_Yu_Karaev,
    COUNT
};

enum class Ocean_Directional_Spreading_Function : uint32_t
{
    Pos_Cos_Squared = 0,
    Mitsuyasu,
    Hasselmann,
    Donelan_Banner,
    COUNT
};

struct Ocean_Spectra_Settings
{
    float wind_speed;
    float wind_direction;
    float fetch;
    Ocean_Spectrum spectrum;
    Ocean_Directional_Spreading_Function directional_spreading_function;
};

struct Ocean_Settings
{
    constexpr static uint32_t MAX_CASCADES = 4;

    uint32_t size = 256;
    uint32_t cascade_count = MAX_CASCADES;
    float length_scales[MAX_CASCADES] = { 28.5327820804f, 74.6997932802f, 195.56659776f, 512.0f }; // L[3] * (1-phi)^(1-idx)
    float gravity = 9.81f;
    float ocean_depth = 35.0f;
    float horizontal_displacement_scale = 1.0f;
    bool swell_enabled = true;
    bool recompute_initial_spectrum = true;
    Ocean_Spectra_Settings local_spectrum;
    Ocean_Spectra_Settings swell_spectrum;
};

struct Ocean_Simulation_Render_Resources;
class Render_Engine;

struct Ocean_Render_Technique_Settings : public Render_Technique_Settings
{
    Ocean_Render_Technique_Settings(
        Render_Engine* render_engine,
        Ocean_Simulation_Render_Resources* resources);

    Ocean_Settings settings;
    virtual void on_gui() override;

private:
    void on_gui_spectrum(
        const char* spectrum_combo_name,
        const char* directional_spread_combo_name,
        const char* wind_speed_name,
        const char* wind_direction_name,
        const char* fetch_name,
        Ocean_Spectra_Settings* spectrum);
    void on_gui_simulation();
    void on_gui_render();
    void on_gui_presets();

    void on_gui_plot();

private:
    int32_t m_last_selected_preset;
    Render_Engine* m_render_engine;
    Ocean_Simulation_Render_Resources* m_resources;

    bool m_recalculate_plot = true;
    using Spectrum_Wavenumber_Vector = std::pair<std::vector<float>, std::vector<float>>;
    std::array<Spectrum_Wavenumber_Vector, Ocean_Settings::MAX_CASCADES> m_cascade_plots;
};
}
