#pragma once

#include "core/defines.hpp"

namespace Raw
{
    class IAllocator
    {
    public:
        virtual ~IAllocator() {}
        [[nodiscard]] virtual void* Allocate(u64 size, u64 alignment = 1) = 0;
        virtual u64 Deallocate(void* ptr) = 0;
    };
}