#pragma once

#include "owge_render_techniques/ocean/ocean_settings.hpp"

namespace owge
{
std::pair<float, float> ocean_calculate_spectrum_sample_for_cascade(
    const Ocean_Settings& settings, uint32_t cascade, uint32_t sample, bool allow_reject = true);

std::pair<std::vector<float>, std::vector<float>> ocean_calculate_spectrum_for_cascade(
    const Ocean_Settings& settings, uint32_t cascade);
}
