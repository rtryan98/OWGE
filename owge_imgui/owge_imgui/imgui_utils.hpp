#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <imgui.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace owge
{
class Render_Engine;
void imgui_init(HWND hwnd, Render_Engine* render_engine);
void imgui_new_frame();
void imgui_shutdown();
}
