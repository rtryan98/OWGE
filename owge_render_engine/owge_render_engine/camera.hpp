#pragma once

#include <DirectXMath.h>

namespace owge
{
using namespace DirectX;

class Input;
enum class Key_Code : uint8_t;
enum class Mouse_Button : uint8_t;

struct Camera_Data
{
    XMFLOAT4X4 proj;
    XMFLOAT4X4 view;
    XMFLOAT4X4 view_proj;
};

struct Fly_Camera_Input_Mapping
{
    Key_Code forward;
    Key_Code backward;
    Key_Code left;
    Key_Code right;
    Key_Code high_speed;
    Key_Code up;
    Key_Code down;
};

struct Simple_Fly_Camera
{
    Camera_Data camera_data;
    Fly_Camera_Input_Mapping input_map;

    float sensitivity;
    float pitch;
    float yaw;
    float movement_speed;

    void update(Input* input, bool active);

private:
    void update_matrices();
};
}
