#pragma once

#include <Windows.h>
#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace owge
{
class Render_Engine;
void imgui_init(HWND hwnd, Render_Engine* render_engine);
void imgui_new_frame();
void imgui_shutdown();

void gui_help_marker(const char* text);
void gui_help_marker_same_line(const char* text);
}
