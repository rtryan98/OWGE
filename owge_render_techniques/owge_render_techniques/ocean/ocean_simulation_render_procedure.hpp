#pragma once

#include <owge_render_engine/render_procedure/render_procedure.hpp>

namespace owge
{
struct Ocean_Settings;
struct Ocean_Simulation_Render_Resources;
class Barrier_Builder;

class Ocean_Simulation_Render_Procedure : public Render_Procedure
{
public:
    Ocean_Simulation_Render_Procedure(
        Ocean_Settings* settings, Ocean_Simulation_Render_Resources* resources);

    virtual void process(const Render_Procedure_Payload& payload) override;

private:
    void process_initial_spectrum(const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder);
    void process_developed_spectrum(const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder);
    void process_ffts(const Render_Procedure_Payload& payload, Barrier_Builder& barrier_builder);

private:
    Ocean_Settings* m_settings;
    Ocean_Simulation_Render_Resources* m_resources;
    float m_time = 0.0f;
};
}
