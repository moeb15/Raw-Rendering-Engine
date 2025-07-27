#include "memory/helpers.hpp"
#include <memory.h>

namespace Raw
{
    void MemoryCopy(void* dst, void* src, u64 size)
    {
        memcpy(dst, src, size);
    }

    u64 MemoryAlign(u64 size, u64 alignment)
    {
        const u64 alignmentMask = alignment - 1;
        return (size + alignmentMask) & ~alignmentMask;
    }
}