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
    float length_scales[MAX_CASCADES] = { 8.0f, 32.0f, 256.0f, 1024.0f };
    float gravity = 9.81f;
    float ocean_depth = 35.0f;
    bool swell_enabled = true;
    Ocean_Spectra_Settings local_spectrum;
    Ocean_Spectra_Settings swell_spectrum;
};

struct Ocean_Render_Technique_Settings : public Render_Technique_Settings
{
    Ocean_Render_Technique_Settings();

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

private:
    int32_t m_last_selected_preset;

    /*
    bool m_dirty_plot = true;
    std::array<uint32_t, Ocean_Settings::MAX_CASCADES> m_plot_cascade_offsets;
    std::vector<float> m_spectrum_values;
    std::vector<float> m_spectrum_rads;
    */
};
}
