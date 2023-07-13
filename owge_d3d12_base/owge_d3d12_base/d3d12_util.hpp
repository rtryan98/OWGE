#pragma once

#include <cstdint>
#include <include/d3d12.h>
#include <exception>
#include <string>
#include <vector>

namespace owge
{
class HRESULT_Exception : public std::exception
{
public:
    HRESULT_Exception(HRESULT hr, const char* message = nullptr);

    [[nodiscard]] virtual const char* what() const override;

private:
    HRESULT m_hr;
    std::string m_message;
};

void throw_if_failed(HRESULT hr, const char* message);

[[nodiscard]] DWORD wait_for_d3d12_fence(ID3D12Fence1* fence, uint64_t value, uint32_t timeout);

[[nodiscard]] DWORD wait_for_d3d12_queue_idle(ID3D12Device* device, ID3D12CommandQueue* queue);

static constexpr uint32_t MAX_RTV_DSV_DESCRIPTORS = 128;

struct Descriptor
{
    D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle;
    D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle;
    uint32_t index;
};

class Descriptor_Allocator
{
public:
    Descriptor_Allocator(ID3D12DescriptorHeap* heap, ID3D12Device* device);

    [[nodiscard]] Descriptor allocate() noexcept;
    void free(uint32_t index) noexcept;

private:
    ID3D12DescriptorHeap* m_heap;
    ID3D12Device* m_device;
    std::vector<uint32_t> m_free_list;
    uint32_t m_current_idx;
};
}
