#include "owge_render_engine/camera.hpp"

#include <owge_window/input.hpp>

namespace owge
{
void Simple_Fly_Camera::update(Input* input, float dt, bool active)
{
    if (!active)
        return;

    update_rotation(input);
    update_position(input, dt);

    XMStoreFloat4x4(&this->camera_data.proj, XMMatrixPerspectiveFovRH(
        XMConvertToRadians(fov_y), aspect, near, far));
    XMStoreFloat4x4(&this->camera_data.view, XMMatrixLookAtRH(
        XMLoadFloat3(&this->position),
        XMVectorAdd(
            XMLoadFloat3(&this->position),
            XMLoadFloat3(&this->forward)),
        XMLoadFloat3(&this->up)));
    XMStoreFloat4x4(&this->camera_data.view_proj, XMMatrixMultiply(
        XMLoadFloat4x4(&this->camera_data.view), XMLoadFloat4x4(&this->camera_data.proj)));
    camera_data.position = { position.x, position.y, position.z, 0.0f };
}

void Simple_Fly_Camera::update_rotation(Input* input)
{
    if (input->is_mouse_pressed(input_map.enable_rotate))
    {
        auto mouse_delta = input->get_mouse_pos_delta();
        yaw -= sensitivity * mouse_delta.x;
        if (yaw > 360.0f)
        {
            yaw -= 360.0f;
        }
        if (yaw < 0.0f)
        {
            yaw += 360.0f;
        }
        pitch -= sensitivity * mouse_delta.y;
        pitch = XMMax(XMMin(pitch, 89.0f), -89.0f);
    }
    forward = {
        XMScalarCos(XMConvertToRadians(yaw)) * XMScalarCos(XMConvertToRadians(pitch)),
        XMScalarSin(XMConvertToRadians(yaw)) * XMScalarCos(XMConvertToRadians(pitch)),
        XMScalarSin(XMConvertToRadians(pitch)),
    };
    XMStoreFloat3(&this->forward,
        XMVector3Normalize(XMLoadFloat3(&this->forward)));
    XMStoreFloat3(&this->right,
        XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&this->forward), XMLoadFloat3(&WORLD_UP))));
    XMStoreFloat3(&this->up,
        XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&this->right), XMLoadFloat3(&this->forward))));
}

void Simple_Fly_Camera::update_position(Input* input, float dt)
{
    bool inp_forward = input->is_key_pressed(input_map.forward);
    bool inp_left = input->is_key_pressed(input_map.left);
    bool inp_back = input->is_key_pressed(input_map.backward);
    bool inp_right = input->is_key_pressed(input_map.right);
    bool inp_down = input->is_key_pressed(input_map.down);
    bool inp_up = input->is_key_pressed(input_map.up);

    float speed = movement_speed * dt;
    if (input->is_key_pressed(input_map.high_speed))
    {
        speed *= 2.0f;
    }

    XMFLOAT3 movement = { 0.0f, 0.0f, 0.0f };

    if (inp_forward && inp_back)
        ;
    else if (inp_forward)
    {
        XMStoreFloat3(&movement,
            XMVectorAdd(XMLoadFloat3(&this->forward), XMLoadFloat3(&movement)));
    }
    else if (inp_back)
    {
        XMStoreFloat3(&movement,
            XMVectorAdd(-1.0f * XMLoadFloat3(&this->forward), XMLoadFloat3(&movement)));
    }

    if (inp_left && inp_right)
        ;
    else if (inp_right)
    {
        XMStoreFloat3(&movement,
            XMVectorAdd(XMLoadFloat3(&this->right), XMLoadFloat3(&movement)));
    }
    else if (inp_left)
    {
        XMStoreFloat3(&movement,
            XMVectorAdd(-1.0f * XMLoadFloat3(&this->right), XMLoadFloat3(&movement)));
    }

    if (inp_up && inp_down)
        ;
    else if (inp_up)
    {
        XMStoreFloat3(&movement,
            XMVectorAdd(XMLoadFloat3(&WORLD_UP), XMLoadFloat3(&movement)));
    }
    else if (inp_down)
    {
        XMStoreFloat3(&movement,
            XMVectorAdd(-1.0f * XMLoadFloat3(&WORLD_UP), XMLoadFloat3(&movement)));
    }

    XMStoreFloat3(&this->position,
        XMVectorAdd(XMLoadFloat3(&this->position), speed * XMVector3Normalize(XMLoadFloat3(&movement))));
}
}
