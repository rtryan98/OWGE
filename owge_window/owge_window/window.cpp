#include "owge_window/window.hpp"

namespace owge
{
HWND Window::get_hwnd() const
{
    return m_hwnd;
}
}
