#include "owge_render_techniques/ocean/ocean_settings.hpp"
#include "owge_render_techniques/ocean/ocean_render_resources.hpp"

#include <bit>
#include <utility>
#include <imgui.h>
#include <implot.h>
#include <string>

namespace owge
{
const char* to_string(Ocean_Spectrum spectrum) noexcept
{
    switch (spectrum)
    {
    case Ocean_Spectrum::Phillips:
        return "Phillips";
    case Ocean_Spectrum::Pierson_Moskowitz:
        return "Pierson Moskowitz";
    case Ocean_Spectrum::Generalized_A_B:
        return "Generalized A, B";
    case Ocean_Spectrum::JONSWAP:
        return "JONSWAP";
    case Ocean_Spectrum::TMA:
        return "Texel MARSEN ARSLOE (TMA)";
    case Ocean_Spectrum::V_Yu_Karaev:
        return "V. Yu. Karaev";
    default:
        std::unreachable();
    }
}

const char* to_string(Ocean_Directional_Spreading_Function directional_spreading_function) noexcept
{
    switch (directional_spreading_function)
    {
    case Ocean_Directional_Spreading_Function::Pos_Cos_Squared:
        return "Positive Cosine Squared";
    case Ocean_Directional_Spreading_Function::Mitsuyasu:
        return "Mitsuyasu";
    case Ocean_Directional_Spreading_Function::Hasselmann:
        return "Hasselmann";
    case Ocean_Directional_Spreading_Function::Donelan_Banner:
        return "Donelan Banner";
    default:
        std::unreachable();
    }
}

bool is_compatible(Ocean_Spectrum spectrum, Ocean_Directional_Spreading_Function directional_spreading_function)
{
    constexpr static bool compatability_table
        [uint32_t(Ocean_Spectrum::COUNT)][uint32_t(Ocean_Directional_Spreading_Function::COUNT)]
    {
        /*                        Pos_Cos_Squared, Mitsuyasu, Hasselmann, Donelan_Banner */
        /* Phillips */          {            true,     false,      false,          false },
        /* Pierson_Moskowitz */ {            true,      true,       true,           true },
        /* Generalized_A_B */   {            true,     false,      false,          false },
        /* JONSWAP */           {            true,      true,       true,           true },
        /* TMA */               {            true,      true,       true,           true },
        /* V_Yu_Karaev */       {            true,      true,       true,           true }
    };
    return compatability_table[uint32_t(spectrum)][uint32_t(directional_spreading_function)];
}

struct Ocean_Simulation_Preset
{
    const char* name;
    Ocean_Settings settings;
};

constexpr static Ocean_Simulation_Preset presets[] = {
    {
        .name = "1 Bft, 4 Cascades, 256x256",
        .settings = {
            .local_spectrum = {
                .wind_speed = 0.7f,
                .wind_direction = 55.0f,
                .fetch = 10000.0f,
                .spectrum = Ocean_Spectrum::V_Yu_Karaev,
                .directional_spreading_function = Ocean_Directional_Spreading_Function::Hasselmann
            },
            .swell_spectrum = {
                .wind_speed = 1.0f,
                .wind_direction = 64.0f,
                .fetch = 100000.0f,
                .spectrum = Ocean_Spectrum::TMA,
                .directional_spreading_function = Ocean_Directional_Spreading_Function::Donelan_Banner
            }
        }
    },
    {
        .name = "2 Bft, 4 Cascades, 256x256",
        .settings = {
            .local_spectrum = {
                .wind_speed = 3.3f,
                .wind_direction = 55.0f,
                .fetch = 10000.0f,
                .spectrum = Ocean_Spectrum::V_Yu_Karaev,
                .directional_spreading_function = Ocean_Directional_Spreading_Function::Hasselmann
            },
            .swell_spectrum = {
                .wind_speed = 3.1f,
                .wind_direction = 64.0f,
                .fetch = 100000.0f,
                .spectrum = Ocean_Spectrum::TMA,
                .directional_spreading_function = Ocean_Directional_Spreading_Function::Donelan_Banner
            }
        }
    },
    {
        .name = "3 Bft, 4 Cascades, 256x256",
        .settings = {
            .local_spectrum = {
                .wind_speed = 5.0f,
                .wind_direction = 55.0f,
                .fetch = 10000.0f,
                .spectrum = Ocean_Spectrum::V_Yu_Karaev,
                .directional_spreading_function = Ocean_Directional_Spreading_Function::Hasselmann
            },
            .swell_spectrum = {
                .wind_speed = 5.4f,
                .wind_direction = 64.0f,
                .fetch = 100000.0f,
                .spectrum = Ocean_Spectrum::TMA,
                .directional_spreading_function = Ocean_Directional_Spreading_Function::Donelan_Banner
            }
        }
    },
};

Ocean_Render_Technique_Settings::Ocean_Render_Technique_Settings(
    Render_Engine* render_engine,
    Ocean_Simulation_Render_Resources* resources)
    : settings(presets[0].settings), m_last_selected_preset(0),
    m_render_engine(render_engine), m_resources(resources)
{}

void Ocean_Render_Technique_Settings::on_gui()
{
    if (ImGui::CollapsingHeader("Ocean Settings"))
    {
        on_gui_simulation();
        ImGui::Separator();
        on_gui_render();
    }
}

void Ocean_Render_Technique_Settings::on_gui_spectrum(
    const char* spectrum_combo_name,
    const char* directional_spread_combo_name,
    const char* wind_speed_name,
    const char* wind_direction_name,
    const char* fetch_name,
    Ocean_Spectra_Settings* spectrum)
{
    ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
    if (ImGui::BeginCombo(spectrum_combo_name, to_string(spectrum->spectrum)))
    {
        for (auto i = 0; i < uint32_t(Ocean_Spectrum::COUNT); ++i)
        {
            auto current_spectrum = Ocean_Spectrum(i);
            if (ImGui::Selectable(to_string(current_spectrum), current_spectrum == spectrum->spectrum))
            {
                spectrum->spectrum = current_spectrum;
                if (!is_compatible(current_spectrum, spectrum->directional_spreading_function))
                {
                    spectrum->directional_spreading_function = Ocean_Directional_Spreading_Function::Pos_Cos_Squared;
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
    if (ImGui::BeginCombo(directional_spread_combo_name, to_string(spectrum->directional_spreading_function)))
    {
        for (auto i = 0; i < uint32_t(Ocean_Directional_Spreading_Function(Ocean_Directional_Spreading_Function::COUNT)); ++i)
        {
            auto current_dir_spread_fn = Ocean_Directional_Spreading_Function(i);
            if (is_compatible(spectrum->spectrum, current_dir_spread_fn))
            {
                if (ImGui::Selectable(to_string(current_dir_spread_fn), current_dir_spread_fn == spectrum->directional_spreading_function))
                {
                    spectrum->directional_spreading_function = current_dir_spread_fn;
                }
            }
        }
        ImGui::EndCombo();
    }
    ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
    ImGui::SliderFloat(wind_speed_name, &spectrum->wind_speed, 0.05f, 40.0f, "%.3f m/s", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
    ImGui::SliderFloat(wind_direction_name, &spectrum->wind_direction, 0.0f, 359.999f, "%.3f deg", ImGuiSliderFlags_AlwaysClamp);
    ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
    ImGui::InputFloat(fetch_name, &spectrum->fetch, 1000.0f, 0.0f, "%.1f");
}

void Ocean_Render_Technique_Settings::on_gui_simulation()
{
    if (ImGui::TreeNode("Simulation Settings"))
    {
        ImGui::Unindent(ImGui::GetTreeNodeToLabelSpacing());
        ImGui::SeparatorText("General Simulation Settings");
        static const char* sizes[] = { "128x128", "256x256", "512x512" };
        uint32_t current_size = settings.size;
        int selected_size = std::countr_zero(settings.size) - std::countr_zero(128u);
        ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
        ImGui::Combo("Size", &selected_size, sizes, IM_ARRAYSIZE(sizes));
        settings.size = 1u << (selected_size + std::countr_zero(128u));
        ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
        ImGui::SliderFloat("Horizontal Displacement Scale", &this->settings.horizontal_displacement_scale, 0.0f, 1.0f);

        if (settings.size != current_size)
        {
            m_resources->resize_textures(m_render_engine, &this->settings);
        }

        static const char* cascade_options[] = { "1", "2", "3", "4" };
        int32_t current_cascade = settings.cascade_count - 1;
        ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
        ImGui::Combo("Cascade Count", &current_cascade, cascade_options, IM_ARRAYSIZE(cascade_options));
        settings.cascade_count = current_cascade + 1;
        for (auto i = 0u; i < settings.cascade_count; ++i)
        {
            std::string cascade_string = "Cascade " + std::to_string(i + 1) + " Lengthscale";
            ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
            ImGui::InputFloat(cascade_string.c_str(), &settings.length_scales[i], 0.5f);
        }
        ImGui::Checkbox("Swell Enabled", &this->settings.swell_enabled);

        ImGui::SeparatorText("Local Energy Spectrum Settings");
        on_gui_spectrum(
            "Spectrum##Local",
            "Directional Spreading Function##Local",
            "Wind Speed##Local",
            "Wind Direction##Local",
            "Fetch##Local",
            &this->settings.local_spectrum);

        if (settings.swell_enabled)
        {
            ImGui::SeparatorText("Swell Energy Spectrum Settings");
            on_gui_spectrum(
                "Spectrum##Swell",
                "Directional Spreading Function##Swell",
                "Wind Speed##Swell",
                "Wind Direction##Swell",
                "Fetch##Swell",
                &this->settings.swell_spectrum);
        }

        ImGui::SeparatorText("Presets");
        ImGui::SetNextItemWidth(IMGUI_ELEMENT_SIZE);
        if (ImGui::BeginCombo("Presets##Ocean_Simulation",
            m_last_selected_preset != -1
            ? presets[m_last_selected_preset].name
            : "No preset selected"))
        {
            for (auto i = 0; i < IM_ARRAYSIZE(presets); ++i)
            {
                if (ImGui::Selectable(presets[i].name, m_last_selected_preset == i))
                {
                    m_last_selected_preset = i;
                }
            }
            ImGui::EndCombo();
        }
        if (ImGui::Button("Apply Preset"))
        {
            if (m_last_selected_preset != -1)
            {
                settings = presets[m_last_selected_preset].settings;
            }
        }

        /*
        ImGui::Separator();

        if (m_dirty_plot)
        {
            m_spectrum_rads.clear();
            m_spectrum_rads.push_back(0.0f);
            m_spectrum_values.clear();
            m_spectrum_values.push_back(0.0f);
            for (auto i = 0; i < Ocean_Settings::MAX_CASCADES; ++i)
            {
                m_plot_cascade_offsets[i] = uint32_t(m_spectrum_values.size() - 1u);
                constexpr static uint32_t PLOT_MAX_SAMPLES = 64;
                for (auto j = 0; j < PLOT_MAX_SAMPLES; ++j)
                {
                    float lengthscale = 2.0f * 3.14f * (j * settings.length_scales[i]);
                    if (i < Ocean_Settings::MAX_CASCADES - 1 && lengthscale > 2.0f * 3.14f / settings.length_scales[i + 1])
                        break;
                    if (i > 0 && lengthscale < 2.0f * 3.14f / settings.length_scales[i - 1])
                        continue;
                    m_spectrum_rads.push_back(lengthscale);
                    m_spectrum_values.push_back(lengthscale);
                }
            }

            m_dirty_plot = false;
        }

        if (ImPlot::BeginPlot("Ocean Energy Spectrum Preview", ImVec2(-1, 0),
            ImPlotFlags_NoFrame))
        {
            constexpr static auto cascade0_fill_color = ImVec4(1.00f, 0.25f, 0.05f, 0.10f);
            constexpr static auto cascade0_line_color = ImVec4(1.00f, 0.25f, 0.05f, 0.75f);

            constexpr static auto cascade1_fill_color = ImVec4(0.15f, 0.21f, 1.00f, 0.10f);
            constexpr static auto cascade1_line_color = ImVec4(0.15f, 0.21f, 1.00f, 0.75f);

            constexpr static auto cascade2_fill_color = ImVec4(0.15f, 0.95f, 0.25f, 0.10f);
            constexpr static auto cascade2_line_color = ImVec4(0.15f, 0.95f, 0.25f, 0.75f);

            constexpr static auto cascade3_fill_color = ImVec4(0.90f, 0.80f, 0.10f, 0.10f);
            constexpr static auto cascade3_line_color = ImVec4(0.90f, 0.80f, 0.10f, 0.75f);

            ImPlot::SetupLegend(ImPlotLocation_North);

            ImPlot::SetupAxis(ImAxis_X1, "omega, rad/s",
                ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoMenus);
            //ImPlot::SetupAxisLimits(ImAxis_X1, 0.0, m_spectrum_rads[m_spectrum_rads.size() - 1], ImPlotCond_Always);

            ImPlot::SetupAxis(ImAxis_Y1, "m^2/rad",
                ImPlotAxisFlags_NoHighlight | ImPlotAxisFlags_NoMenus);
            //ImPlot::SetupAxisLimits(ImAxis_Y1, 0.0, m_spectrum_values[m_spectrum_values.size() - 1], ImPlotCond_Always);

            ImPlot::SetNextFillStyle(cascade0_fill_color);
            ImPlot::SetNextLineStyle(cascade0_line_color, 2.0f);
            ImPlot::PlotLine("Cascade 1", m_spectrum_rads.data(), m_spectrum_values.data(),
                m_plot_cascade_offsets[1], ImPlotLineFlags_Shaded, m_plot_cascade_offsets[0]);

            ImPlot::SetNextFillStyle(cascade1_fill_color);
            ImPlot::SetNextLineStyle(cascade1_line_color, 2.0f);
            ImPlot::PlotLine("Cascade 2", m_spectrum_rads.data(), m_spectrum_values.data(),
                m_plot_cascade_offsets[2], ImPlotLineFlags_Shaded, m_plot_cascade_offsets[1]);

            ImPlot::SetNextFillStyle(cascade2_fill_color);
            ImPlot::SetNextLineStyle(cascade2_line_color, 2.0f);
            ImPlot::PlotLine("Cascade 3", m_spectrum_rads.data(), m_spectrum_values.data(),
                m_plot_cascade_offsets[3], ImPlotLineFlags_Shaded, m_plot_cascade_offsets[2]);

            ImPlot::SetNextFillStyle(cascade3_fill_color);
            ImPlot::SetNextLineStyle(cascade3_line_color, 2.0f);
            ImPlot::PlotLine("Cascade 4", m_spectrum_rads.data(), m_spectrum_values.data(),
                uint32_t(m_spectrum_rads.size() - 1u), ImPlotLineFlags_Shaded, m_plot_cascade_offsets[3]);
            ImPlot::EndPlot();
        }
        */
        ImGui::TreePop();
    }
}

void Ocean_Render_Technique_Settings::on_gui_render()
{
    if (ImGui::TreeNode("Render Settings"))
    {
        ImGui::TreePop();
    }
}
}
