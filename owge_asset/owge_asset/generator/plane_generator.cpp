#include "owge_asset/generator/plane_generator.hpp"

namespace owge
{
Generated_Simple_Mesh<XMFLOAT2> mesh_generate_simple_plane_2d(uint32_t size)
{
    Generated_Simple_Mesh<XMFLOAT2> result;

    result.vertex_positions.reserve(size * size);
    result.indices.reserve(((size - 1) * (size -1)) * 6);

    for (uint32_t i = 0; i < size; ++i)
    {
        for (uint32_t j = 0; j < size; ++j)
        {
            auto& pos = result.vertex_positions.emplace_back();
            pos = { float(j), float(i) };
        }
    }

    auto push_triangle_indices = [&](uint32_t a, uint32_t b, uint32_t c) mutable {
        result.indices.push_back(a);
        result.indices.push_back(b);
        result.indices.push_back(c);
    };

    for (uint32_t i = 0; i < size - 1; ++i)
    {
        for (uint32_t j = 0; j < size - 1; ++j)
        {
            auto diff = size;
            auto idx = i * diff + j;
            push_triangle_indices(
                idx,
                idx + 1,
                idx + 1 + diff);
            push_triangle_indices(
                idx,
                idx + 1 + diff,
                idx + diff);
        }
    }

    return result;
}
}
