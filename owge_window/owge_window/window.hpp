#pragma once

#include <Windows.h>

namespace owge
{
class Window
{
public:
    [[nodiscard]] HWND get_hwnd() const;

private:
    HWND m_hwnd;

};
}
