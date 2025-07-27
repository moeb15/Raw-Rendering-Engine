#pragma once

#include "core/defines.hpp"

namespace Raw
{
    void MemoryCopy(void* dst, void* src, u64 size);
    u64 MemoryAlign(u64 size, u64 alignment);

    struct MemoryStats
    {
        u64 allocatedBytes{ 0 };
        u64 totalBytes{ 0 };
        u32 allocationCount{ 0 };

        void Add(u64 alloc)
        {
            if(alloc)
            {
                allocatedBytes += alloc;
                allocationCount++;
            }
        }
    };
}