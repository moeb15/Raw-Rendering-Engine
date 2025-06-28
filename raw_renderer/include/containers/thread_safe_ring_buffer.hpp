#pragma once

#include "core/defines.hpp"
#include <mutex>

namespace Raw
{
    // implementation from https://wickedengine.net/2018/11/simple-job-system-using-standard-c/
    template <typename T, u64 capacity>
    class ThreadSafeRingBuffer
    {
    public:
        RAW_INLINE bool Push(const T& item)
        {
            bool res = false;
            {
                std::scoped_lock<std::mutex> lock(m_Lock);
                u64 next = (m_Head + 1) % capacity;
                if(next != m_Tail)
                {
                    m_Data[m_Head] = item;
                    m_Head = next;
                    res = true;
                }
            }
            return res;
        }

        RAW_INLINE bool Pop(T& item)
        {
            bool res = false;
            {
                std::scoped_lock<std::mutex> lock(m_Lock);
                if(m_Tail != m_Head)
                {
                    item = m_Data[m_Tail];
                    m_Tail = (m_Tail + 1) % capacity;
                    res = true;
                }
            }
            return res;
        }
    
    private:
        T m_Data[capacity];
        u64 m_Head{ 0 };
        u64 m_Tail{ 0 };
        std::mutex m_Lock;
    };
}