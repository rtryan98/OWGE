#include "owge_d3d12_base/d3d12_util.hpp"

#include <wrl.h>
#include <sstream>

namespace owge
{
HRESULT_Exception::HRESULT_Exception(HRESULT hr, const char* message)
    : std::exception(), m_hr(hr), m_message()
{
    std::stringstream strstream;
    strstream << "HRESULT error: " << std::hex << m_hr;
    if (message != nullptr)
    {
        strstream << "\t\t" << message;
    }
    m_message = strstream.str();
}

const char* HRESULT_Exception::what() const
{
    return m_message.c_str();
}

void throw_if_failed(HRESULT hr, const char* message)
{
    if (FAILED(hr))
    {
        throw HRESULT_Exception(hr, message);
    }
}

DWORD wait_for_d3d12_fence(ID3D12Fence1* fence, uint64_t value, uint32_t timeout)
{
    DWORD result = WAIT_FAILED;
    if (fence->GetCompletedValue() < value)
    {
        HANDLE event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);
        throw_if_failed(fence->SetEventOnCompletion(value, event_handle),
            "Error setting fence event in wait_for_d3d12_fence.");
        if (event_handle != 0)
        {
            result = WaitForSingleObject(event_handle, timeout);
            CloseHandle(event_handle);
        }
    }
    return result;
}

DWORD wait_for_d3d12_queue_idle(ID3D12Device* device, ID3D12CommandQueue* queue)
{
    using Microsoft::WRL::ComPtr;
    constexpr static uint64_t FENCE_SIGNAL_VALUE = 1;

    ComPtr<ID3D12Fence1> fence = nullptr;
    throw_if_failed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)),
        "Error creating Fence for wait_for_d3d12_queue_idle.");
    throw_if_failed(queue->Signal(fence.Get(), FENCE_SIGNAL_VALUE),
        "Error signalling Fence for wait_for_d3d12_queue_idle.");
    return wait_for_d3d12_fence(fence.Get(), FENCE_SIGNAL_VALUE, INFINITE);
}
}
