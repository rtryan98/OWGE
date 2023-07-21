#pragma once

#include <vector>
#include <DirectXMath.h>

namespace owge
{
using namespace DirectX;

template<typename Dimension, typename Index_Type = uint32_t>
struct Generated_Simple_Mesh
{
    std::vector<Dimension> vertex_positions;
    std::vector<Index_Type> indices;
};

template<typename Dimension, typename Vertex_Data, typename Index_Type = uint32_t>
struct Generated_Mesh
{
    std::vector<Dimension> vertex_positions;
    std::vector<Vertex_Data> vertex_data;
    std::vector<Index_Type> indices;
};
}
