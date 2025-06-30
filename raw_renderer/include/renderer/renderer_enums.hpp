#pragma once

#include "core/service.hpp"

namespace Raw::GFX
{
    enum class EDeviceBackend : u8
    {
        VULKAN,
        D3D12
    };
}