#pragma once

#include "core/defines.hpp"
#include <wyhash.h>

namespace Raw::Utils
{
    RAW_INLINE u64 HashCString(const cstring& value, u64 seed = 0)
    {
        return wyhash(value, strlen(value), seed, _wyp);
    }
}