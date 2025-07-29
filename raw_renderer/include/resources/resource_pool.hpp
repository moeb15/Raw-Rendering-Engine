#pragma once

#include "core/defines.hpp"
#include "core/asserts.hpp"
#include "memory/memory_service.hpp"
#include <bitset>
#include <string.h>

namespace Raw
{
    constexpr u32 INVALID_INDEX = U32_MAX;
    constexpr u32 MAX_POOL_SIZE = 1 << 16;

    // based on ResourcePool implementation from Mastering Graphics Programming with Vulkan
    template <typename T>
    class ResourcePool
    {
    public:
        ResourcePool() {}
        ~ResourcePool() {}

        void Init(u32 poolSize);
        void Shutdown();
        
        u32 ObtainResource();
        void ReleaseResource(u32 index);
        void FreeAllResources();

        T* GetResource(u32 index);
        const T* GetResource(u32 index) const;

        RAW_INLINE u32 GetPoolSize() const { return m_PoolSize; }
        RAW_INLINE std::bitset<MAX_POOL_SIZE>& GetBitset() { return m_AliveHandles; }

    private:
        u8* m_Data;
        u32* m_FreeIndices;
        u32 m_PoolSize{ 0 };
        u32 m_ResourceSize{ 0 };
        u32 m_UsedIndices{ 0 };
        u32 m_FreeIndicesHead{ 0 };
        std::bitset<MAX_POOL_SIZE> m_AliveHandles;
        
    };

    template <typename T>
    RAW_INLINE void ResourcePool<T>::Init(u32 poolSize)
    {
        RAW_ASSERT_MSG(poolSize <= MAX_POOL_SIZE, "ResourcePool exceeds max size of %u!", MAX_POOL_SIZE);
        m_ResourceSize = sizeof(T);
        m_PoolSize = poolSize;

        u64 allocSize = m_PoolSize * (m_ResourceSize + sizeof(u32));
        m_Data = (u8*)RAW_ALLOCATE(allocSize, 1);

        m_FreeIndices = (u32*)(m_Data + poolSize * m_ResourceSize);
        m_FreeIndicesHead = 0;

        for(u32 i = 0; i < m_PoolSize; i++)
        {
            m_FreeIndices[i] = i;
        }

        m_UsedIndices = 0;
    }

    template <typename T>
    RAW_INLINE void ResourcePool<T>::Shutdown()
    {
        RAW_ASSERT_MSG(m_FreeIndicesHead == 0, "Resource pool has unfreed resources!");
        RAW_ASSERT(m_UsedIndices == 0);     
        RAW_DEALLOCATE(m_Data);
    }

    template <typename T>
    RAW_INLINE u32 ResourcePool<T>::ObtainResource()
    {
        if(m_FreeIndicesHead < m_PoolSize)
        {
            const u32 freeIndex = m_FreeIndices[m_FreeIndicesHead++];
            m_UsedIndices++;
            m_AliveHandles.set(freeIndex, true);
            return freeIndex;
        }
        RAW_ASSERT(false);
        return INVALID_INDEX;   
    }

    template <typename T>
    RAW_INLINE void ResourcePool<T>::FreeAllResources()
    {
        m_FreeIndicesHead = 0;
        m_UsedIndices = 0;
        
        for(u32 i = 0; i < m_PoolSize; i++)
        {
            m_FreeIndices[i] = i;
            m_AliveHandles.set(i, false);
        }
    }

    template <typename T>
    RAW_INLINE void ResourcePool<T>::ReleaseResource(u32 handle)
    {
        m_FreeIndices[--m_FreeIndicesHead] = handle;
        m_AliveHandles.set(handle, false);
        m_UsedIndices--;
    }

    template <typename T>
    RAW_INLINE T* ResourcePool<T>::GetResource(u32 handle)
    {
        if(handle != INVALID_INDEX)
        {
            return reinterpret_cast<T*>(m_Data + handle * m_ResourceSize);
        }
        return nullptr;
    }

    template <typename T>
    RAW_INLINE const T* ResourcePool<T>::GetResource(u32 handle) const 
    {
        if(handle != INVALID_INDEX)
        {
            return reinterpret_cast<T*>(m_Data + handle * m_ResourceSize);
        }
        return nullptr;
    }
}