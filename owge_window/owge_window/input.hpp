#pragma once
#include <array>
#include <cstdint>
#include <DirectXMath.h>

typedef struct HWND__* HWND;

namespace owge
{
using namespace DirectX;

enum class Key_Code : uint8_t;
enum class Mouse_Button : uint8_t;

class Input
{
public:
    Input(HWND hwnd);

    void update_input_state();

    bool is_key_released(Key_Code key) const noexcept;
    bool is_key_pressed(Key_Code key) const noexcept;
    bool is_key_clicked(Key_Code key) const noexcept;

    bool is_mouse_released(Mouse_Button mb) const noexcept;
    bool is_mouse_pressed(Mouse_Button mb) const noexcept;
    bool is_mouse_clicked(Mouse_Button mb) const noexcept;

    const XMFLOAT2 get_mouse_pos() const noexcept;
    const XMFLOAT2 get_mouse_pos_delta() const noexcept;

private:
    HWND m_hwnd;
    std::array<uint8_t, 256> m_current_state = {};
    std::array<uint8_t, 256> m_last_state = {};
    XMFLOAT2 m_current_mouse_pos = {};
    XMFLOAT2 m_last_mouse_pos = {};
};
}
