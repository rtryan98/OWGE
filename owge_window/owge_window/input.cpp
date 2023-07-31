#include "owge_window/input.hpp"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace owge
{
Input::Input(HWND hwnd)
    : m_hwnd(hwnd)
{}

void Input::update_input_state()
{
    m_last_state = m_current_state;
    GetKeyboardState(m_current_state.data());

    m_last_mouse_pos = m_current_mouse_pos;
    POINT cursor_pos;
    GetCursorPos(&cursor_pos);
    ScreenToClient(m_hwnd, &cursor_pos);
    m_current_mouse_pos = { float(cursor_pos.x), float(cursor_pos.y) };
}

uint8_t high_bit(uint8_t x)
{
    return x & 0x80;
}

bool Input::is_key_released(Key_Code key) const noexcept
{
    return high_bit(m_last_state[static_cast<uint8_t>(key)])
        && !high_bit(m_current_state[static_cast<uint8_t>(key)]);
}

bool Input::is_key_pressed(Key_Code key) const noexcept
{
    return high_bit(m_current_state[static_cast<uint8_t>(key)]);
}

bool Input::is_key_clicked(Key_Code key) const noexcept
{
    return !high_bit(m_last_state[static_cast<uint8_t>(key)])
        && high_bit(m_current_state[static_cast<uint8_t>(key)]);
}

bool Input::is_mouse_released(Mouse_Button mb) const noexcept
{
    return high_bit(m_last_state[static_cast<uint8_t>(mb)])
        && !high_bit(m_current_state[static_cast<uint8_t>(mb)]);
}

bool Input::is_mouse_pressed(Mouse_Button mb) const noexcept
{
    return high_bit(m_current_state[static_cast<uint8_t>(mb)]);
}

bool Input::is_mouse_clicked(Mouse_Button mb) const noexcept
{
    return !high_bit(m_last_state[static_cast<uint8_t>(mb)])
        && high_bit(m_current_state[static_cast<uint8_t>(mb)]);
}

const XMFLOAT2 Input::get_mouse_pos() const noexcept
{
    return m_current_mouse_pos;
}

const XMFLOAT2 Input::get_mouse_pos_delta() const noexcept
{
    return { m_current_mouse_pos.x - m_last_mouse_pos.x, m_current_mouse_pos.y - m_last_mouse_pos.y };
}
}
