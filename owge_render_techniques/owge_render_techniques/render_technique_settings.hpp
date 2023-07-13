#pragma once

namespace owge
{
constexpr static float IMGUI_ELEMENT_SIZE = 256.0f;

class Render_Technique_Settings
{
public:
    virtual void on_gui() = 0;
};
}
