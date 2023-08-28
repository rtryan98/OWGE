#pragma once

#include <owge_window/input_codes.hpp>
#include <DirectXMath.h>

#undef near
#undef far

namespace owge
{
using namespace DirectX;

class Input;

struct Camera_Data
{
    XMFLOAT4X4 proj;
    XMFLOAT4X4 view;
    XMFLOAT4X4 view_proj;
    XMFLOAT4 position;
};

struct Fly_Camera_Input_Mapping
{
    Key_Code forward = Key_Code::Key_W;
    Key_Code backward = Key_Code::Key_S;
    Key_Code left = Key_Code::Key_A;
    Key_Code right = Key_Code::Key_D;
    Key_Code high_speed = Key_Code::Key_Left_Shift;
    Key_Code up = Key_Code::Key_Q;
    Key_Code down = Key_Code::Key_E;
    Mouse_Button enable_rotate = Mouse_Button::Mouse_Left;
};

struct Simple_Fly_Camera
{
    constexpr static XMFLOAT3 WORLD_UP = { 0.0f, 0.0f, 1.0f };

    Camera_Data camera_data;
    Fly_Camera_Input_Mapping input_map;

    float fov_y;
    float aspect;
    float near;
    float far;

    float sensitivity;
    float movement_speed;
    float pitch;
    float yaw;

    XMFLOAT3 position;
    XMFLOAT3 forward;
    XMFLOAT3 right;
    XMFLOAT3 up;

    void update(Input* input, float dt, bool active);

private:
    void update_rotation(Input* input);
    void update_position(Input* input, float dt);
};
}
