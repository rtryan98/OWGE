#pragma once

#include <owge_render_engine/render_procedure/render_procedure.hpp>

namespace owge
{
class Imgui_Render_Procedure : public Render_Procedure
{
public:
    Imgui_Render_Procedure();

    virtual void process(const Render_Procedure_Payload& payload) override;
};
}
