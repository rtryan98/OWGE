#pragma once

#include <owge_render_engine/render_procedure/render_procedure.hpp>

namespace owge
{
struct Ocean_Settings;
struct Ocean_Simulation_Render_Resources;

class Ocean_Surface_Render_Procedure : public Render_Procedure
{
public:
    Ocean_Surface_Render_Procedure(
        Ocean_Settings* settings, Ocean_Simulation_Render_Resources* resources);

    virtual void process(const Render_Procedure_Payload& payload) override;

private:
    Ocean_Settings* m_settings;
    Ocean_Simulation_Render_Resources* m_resources;
};
}
