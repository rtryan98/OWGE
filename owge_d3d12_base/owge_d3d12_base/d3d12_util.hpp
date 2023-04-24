#pragma once

#include <cstdint>
#include <d3d12.h>
#include <exception>
#include <string>

namespace owge
{
class HRESULT_Exception : public std::exception
{
public:
    HRESULT_Exception(HRESULT hr);

    [[nodiscard]] virtual const char* what() const override;

private:
    HRESULT m_hr;
    std::string m_message;
};

void throw_if_failed(HRESULT hr);

[[nodiscard]] DWORD wait_for_d3d12_fence(ID3D12Fence1* fence, uint64_t value, uint32_t timeout);

[[nodiscard]] DWORD wait_for_d3d12_queue_idle(ID3D12Device* device, ID3D12CommandQueue* queue);
}
