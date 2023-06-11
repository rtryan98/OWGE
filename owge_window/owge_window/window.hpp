#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <Windows.h>

namespace owge
{
struct Window_Settings
{
    uint32_t width;
    uint32_t height;
    std::string_view title;
    WNDPROC userproc;
};

struct Window_Data
{
    uint32_t width;
    uint32_t height;
    std::string title;
    WNDPROC userproc;
    bool alive;
};

class Window
{
public:
    Window(const Window_Settings& settings);
    ~Window();

    void poll_events();

    [[nodiscard]] HWND get_hwnd() const
    {
        return m_hwnd;
    }
    [[nodiscard]] const Window_Data& get_data() const
    {
        return m_data;
    }

private:
    HWND m_hwnd;
    Window_Data m_data;
};
}
