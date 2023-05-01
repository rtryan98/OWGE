#pragma once

#include <wrl.h>

namespace owge
{
template<typename T>
using Com_Ptr = Microsoft::WRL::ComPtr<T>;
}
