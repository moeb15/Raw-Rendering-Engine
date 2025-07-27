#include "memory/allocators/heap_allocator.hpp"
#include "memory/helpers.hpp"
#include "core/asserts.hpp"
#include <tlsf.h>
#include <stdlib.h>

// heap allocator implementaion based on the book Mastering Graphics Programming With Vulkan 
namespace Raw
{
    static void ExitWalker(void* ptr, u64 size, int used, void* user)
    {
        MemoryStats* stats = (MemoryStats*)user;
        stats->Add(used ? size : 0);
        
        if(used) RAW_INFO("Found active allocator %p, %llu bytes\n", ptr, size);
    }

    HeapAllocator::~HeapAllocator() {}

    void HeapAllocator::Init(u64 size)
    {
        m_Data = malloc(size);
        m_MaxSize = size;
        m_AllocatedSize = 0;
        m_tlsfHandle = tlsf_create_with_pool(m_Data, size);

        RAW_INFO("HeapAllocator of size %llu bytes created\n", size);
    }

    void HeapAllocator::Shutdown()
    {
        MemoryStats stats{ 0, m_MaxSize };
        pool_t pool = tlsf_get_pool(m_tlsfHandle);
        tlsf_walk_pool(pool, ExitWalker, (void*)&stats);

        if(stats.allocatedBytes)
        {
            RAW_ASSERT_MSG(false, "HeapAllocator Shutdown.\n===============\nFAILURE! Allocated memory detected. allocated %llu, total %llu\n===============\n\n", stats.allocatedBytes, stats.totalBytes);
        }
        else
        {
            RAW_INFO("HeapAllocator Shutdown - all memory free!");
        }

        RAW_ASSERT_MSG(stats.allocatedBytes == 0, "HeapAllocator Shutdown \n===============\n Allocations still present.")
        
        tlsf_destroy(m_tlsfHandle);
        free(m_Data);
    }

    void* HeapAllocator::Allocate(u64 size, u64 alignment)
    {
        void* allocatedMemory = alignment == 1 ? tlsf_malloc(m_tlsfHandle, size) : tlsf_memalign(m_tlsfHandle, alignment, size);
        u64 actualSize = tlsf_block_size(allocatedMemory);
        m_AllocatedSize += actualSize;
        
        return allocatedMemory;
    }

    u64 HeapAllocator::Deallocate(void* ptr)
    {
        u64 actualSize = tlsf_block_size(ptr);
        m_AllocatedSize -= actualSize;
        tlsf_free(m_tlsfHandle, ptr);
        
        return actualSize;
    }
}