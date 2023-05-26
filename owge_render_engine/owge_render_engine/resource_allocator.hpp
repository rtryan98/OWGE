#pragma once

#include "owge_render_engine/resource.hpp"

#include <vector>

namespace owge
{
template<typename T>
class Resource_Allocator
{
    using Handle_Type = Base_Resource_Handle<T>;
    static constexpr uint32_t NO_HEAD = ~0u;

public:
    Resource_Allocator(std::size_t size) noexcept
        : m_head(NO_HEAD), m_storage()
    {
        m_storage.reserve(size);
    }

    [[nodiscard]] Handle_Type insert(
        uint8_t flags, const T& value) noexcept
    {
        uint32_t idx = 0;
        if (m_head != NO_HEAD)
        {
            idx = m_head;
            m_head = m_storage[idx].next;
        }
        else
        {
            m_storage.push_back({});
            idx = uint32_t(m_storage.size() - 1ull);
        }

        auto& data = m_storage[idx];
        data.alive = true;
        data.flags = flags;
        data.element = value;
        return Handle_Type {
            .alive = true,
            .flags = flags,
            .gen = data.gen,
            .idx = idx
        };
    }

    void remove(Handle_Type handle) noexcept
    {
        auto& data = m_storage[handle.idx];
        data.alive = false;
        data.flags = 0;
        data.gen += 1;
        data.element = {};
        m_head = handle.idx;
    }

    [[nodiscard]] T& operator[](Handle_Type handle) noexcept
    {
        return m_storage[handle.idx].element;
    }

    [[nodiscard]] const T& at(Handle_Type handle) const noexcept
    {
        return m_storage.at(handle.idx).element;
    }

private:
    uint32_t m_head;
    struct Storage
    {
        uint32_t alive : 1;
        uint32_t flags : 8;
        uint32_t gen : 23;
        uint32_t next;
        T element;
    };
    std::vector<Storage> m_storage;
};
}
