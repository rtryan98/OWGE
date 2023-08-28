#pragma once

#include "owge_render_engine/render_procedure/render_procedure.hpp"
#include "owge_render_engine/resource.hpp"
#include <vector>

namespace owge
{
struct Swapchain_Pass_Settings
{
    float clear_color[4];
    Texture_Handle depth_stencil_texture;
};

class Swapchain_Pass_Render_Procedure : public Render_Procedure
{
public:
    Swapchain_Pass_Render_Procedure(const Swapchain_Pass_Settings& settings);

    void add_subprocedure(Render_Procedure* sub_procedure);
    virtual void process(const Render_Procedure_Payload& payload) override;

private:
    Swapchain_Pass_Settings m_settings;
    std::vector<Render_Procedure*> m_sub_procedures = {};
};
}
