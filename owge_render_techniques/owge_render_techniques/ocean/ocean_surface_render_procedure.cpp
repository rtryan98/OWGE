#include "owge_render_techniques/ocean/ocean_surface_render_procedure.hpp"
#include "owge_render_techniques/ocean/ocean_render_resources.hpp"

#include <owge_render_engine/command_list.hpp>

namespace owge
{
Ocean_Surface_Render_Procedure::Ocean_Surface_Render_Procedure(Ocean_Settings* settings, Ocean_Simulation_Render_Resources* resources)
    : Render_Procedure("Ocean_Surface_Render")
    , m_settings(settings)
    , m_resources(resources)
{}

void Ocean_Surface_Render_Procedure::process(const Render_Procedure_Payload& payload)
{
    payload.cmd->set_pipeline_state(m_resources->surface_plane_pso);
    payload.cmd->set_bindset_graphics(m_resources->surface_render_vs_bindset, 0);
    payload.cmd->set_bindset_graphics(m_resources->surface_render_ps_bindset, 2);

}
}
