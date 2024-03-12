#ifndef OWGE_CAMERA
#define OWGE_CAMERA

struct Camera
{
    float4x4 proj;
    float4x4 proj_inv;
    float4x4 view;
    float4x4 view_inv;
    float4x4 view_proj;
    float4x4 view_proj_inv;
    float4 pos;
};

#endif // OWGE_CAMERA
