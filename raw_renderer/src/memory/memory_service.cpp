#include "memory/memory_service.hpp"
#include "core/asserts.hpp"
#include <string.h>

namespace Raw
{
    static MemoryService s_MemoryService;

    MemoryService* MemoryService::Get()
    {
        return &s_MemoryService;
    }

    void MemoryService::Init(void* config)
    {
        RAW_INFO("MemoryService initializing...");

        MemoryConfig memConfig = *(MemoryConfig*)config;
        RAW_ASSERT_MSG(memConfig.maxSize > 0, "Must intiialize MemoryService with valid amount of bytes, %llu bytes provided", memConfig.maxSize);

        m_MaxSize = memConfig.maxSize;
        m_HeapAllocator.Init(memConfig.maxSize);
    }

    void MemoryService::Shutdown()
    {
        RAW_INFO("MemoryService shutting down...");
        m_HeapAllocator.Shutdown();
    }

    void* MemoryService::Allocate(u64 size, u64 alignment)
    {
        return m_HeapAllocator.Allocate(size, alignment);
    }

    void MemoryService::Deallocate(void* ptr)
    {
        u64 size = m_HeapAllocator.Deallocate(ptr);
        ptr = nullptr;
    }
}