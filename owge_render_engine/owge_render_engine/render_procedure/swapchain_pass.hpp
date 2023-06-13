#pragma once

#include "owge_render_engine/render_procedure/render_procedure.hpp"
#include <vector>

namespace owge
{
struct Swapchain_Pass_Settings
{
    float clear_color[4];
};

class Swapchain_Pass : public Render_Procedure
{
public:
    Swapchain_Pass(const Swapchain_Pass_Settings& settings);

    void add_subprocedure();
    virtual void process(const Render_Procedure_Payload& payload) override;

private:
    Swapchain_Pass_Settings m_settings;
    std::vector<Render_Procedure*> m_sub_procedures = {};
};
}
