#pragma once

#include "core/defines.hpp"
#include "core/service.hpp"
#include "memory/allocators/heap_allocator.hpp"
#include "memory/helpers.hpp"

namespace Raw
{
    struct MemoryConfig
    {
        u64 maxSize{ RAW_MB(32) };
    };

    class MemoryService : public IService
    {
    public:
        DISABLE_COPY(MemoryService);

        MemoryService() {}
        ~MemoryService() {}

        RAW_DECLARE_SERVICE(MemoryService);

        virtual void Init(void* config) override;
        virtual void Shutdown() override;

        [[nodiscard]] void* Allocate(u64 size, u64 alignment = 1);
        void Deallocate(void* ptr);

        static constexpr cstring k_ServiceName = "raw_memory_service";

    private:
        u64 m_MaxSize;
        HeapAllocator m_HeapAllocator;
    
    };

#define RAW_ALLOCATE(size, alignment) Raw::MemoryService::Get()->Allocate(size, alignment)
#define RAW_DEALLOCATE(ptr) Raw::MemoryService::Get()->Deallocate(ptr)
}