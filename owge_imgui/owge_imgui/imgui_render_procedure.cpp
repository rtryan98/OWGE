#include "owge_imgui/imgui_render_procedure.hpp"
#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <owge_render_engine/command_list.hpp>

namespace owge
{
Imgui_Render_Procedure::Imgui_Render_Procedure()
    : Render_Procedure("Imgui_Render")
{}

void Imgui_Render_Procedure::process(const Render_Procedure_Payload& payload)
{
    ImGui::EndFrame();
    ImGui::Render();
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), payload.cmd->d3d12_cmd());
}
}
