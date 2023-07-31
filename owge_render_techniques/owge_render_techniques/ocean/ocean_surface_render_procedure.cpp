#include "owge_render_techniques/ocean/ocean_surface_render_procedure.hpp"
#include "owge_render_techniques/ocean/ocean_render_resources.hpp"

#include <owge_render_engine/render_engine.hpp>
#include <owge_render_engine/command_list.hpp>
#include <owge_render_engine/camera.hpp>

namespace owge
{
Ocean_Surface_Render_Procedure::Ocean_Surface_Render_Procedure(
    Ocean_Settings* settings,
    Ocean_Simulation_Render_Resources* resources,
    Camera_Data* camera)
    : Render_Procedure("Ocean_Surface_Render")
    , m_settings(settings)
    , m_resources(resources)
    , m_camera(camera)
{}

void Ocean_Surface_Render_Procedure::process(const Render_Procedure_Payload& payload)
{
    Ocean_Surface_VS_Render_Data vs_render_data = {
        .view_proj = m_camera->view_proj
    };
    payload.render_engine->copy_and_upload_data(
        sizeof(Ocean_Surface_VS_Render_Data),
        0,
        m_resources->ocean_surface_vs_render_data_buffer,
        0,
        &vs_render_data);

    payload.cmd->set_pipeline_state(m_resources->surface_plane_pso);
    payload.cmd->set_index_buffer(m_resources->ocean_surface_index_buffer, Index_Type::Uint32);
    payload.cmd->set_primitive_topology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    payload.cmd->set_bindset_graphics(m_resources->surface_render_vs_bindset, 0);
    payload.cmd->set_bindset_graphics(m_resources->surface_render_ps_bindset, 2);

    payload.cmd->draw_indexed(m_resources->ocean_surface_index_count, 0, 1, 0, 0);
}
}
