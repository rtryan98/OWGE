#include "owge_asset/generator/plane_generator.hpp"

namespace owge
{
Generated_Simple_Mesh<XMFLOAT2> mesh_generate_simple_plane_2d(uint32_t width, uint32_t length)
{
    Generated_Simple_Mesh<XMFLOAT2> result;

    result.vertex_positions.reserve(width * length);
    result.indices.reserve(((width - 1) * (length -1)) * 6);

    auto push_triangle_indices = [&](uint32_t a, uint32_t b, uint32_t c) mutable {
        result.indices.push_back(a);
        result.indices.push_back(b);
        result.indices.push_back(c);
    };

    for (uint32_t i = 0; i < width; ++i)
    {
        for (uint32_t j = 0; j < length; ++j)
        {
            auto& pos = result.vertex_positions.emplace_back();
            pos = { float(i), float(j) };

            if (i < width - 1 && j < length - 1)
            {
                push_triangle_indices(j, j+1, j+1+i*width);
                push_triangle_indices(j, j+1+i*width, j+i*width);
            }
        }
    }

    return result;
}
}
