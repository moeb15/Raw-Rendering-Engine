#pragma once

#include "core/defines.hpp"
#include "memory/allocators/allocators.hpp"

namespace Raw
{
    class HeapAllocator : public IAllocator
    {
    public:
        ~HeapAllocator() override;
        void Init(u64 size);
        void Shutdown();

        virtual void* Allocate(u64 size, u64 alignment = 1) override;
        virtual u64 Deallocate(void* ptr) override;

    private:
        void* m_tlsfHandle;
        void* m_Data;
        u64 m_AllocatedSize{ 0 };
        u64 m_MaxSize{ 0 };
        
    };
}