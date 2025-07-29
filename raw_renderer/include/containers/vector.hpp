#pragma once

#include "core/defines.hpp"
#include "core/asserts.hpp"
#include "memory/memory_service.hpp"
#include "memory/helpers.hpp"
#include <utility>

namespace Raw::rstd
{
    template <typename T>
    class vector
    {
    public:
        vector() { }
        ~vector() { shutdown(); }

        void shutdown();

        void reserve(u32 capacity);
        void resize(u32 size);
        void clear();

        void push_back(const T& element);
        void push_back(T&& element);
        
        template <typename... Args>
        void emplace_back(Args&&... args);

        T& push();
        T& pop_back();
        void delete_swap(u32 index);

        T& operator[](u32 index);
        const T& operator[](u32 index) const;

        u32 size() const;
        u32 capacity() const;
        T* data() const;

        T& back();
        const T& back() const;
        T& front();
        const T& front() const;

    private:
        void grow(u32 newCapacity);

    private:
        T* m_Data{ nullptr };
        u32 m_Size{ 0 };
        u32 m_Capacity{ 0 };    
    
    };

    template <typename T>
    void vector<T>::shutdown()
    {
        if(m_Capacity > 0) RAW_DEALLOCATE(m_Data);
        
        m_Data = nullptr;
        m_Size = 0;
        m_Capacity = 0;
    }

    template <typename T>
    void vector<T>::push_back(const T& elem)
    {
        if(m_Size >= m_Capacity) grow(m_Capacity + 1);

        m_Data[m_Size++] = elem;
    }

    template <typename T>
    void vector<T>::push_back(T&& elem)
    {
        if(m_Size >= m_Capacity) grow(m_Capacity + 1);

        m_Data[m_Size++] = std::move(elem);
    }

    template <typename T>
    template <typename... Args>
    void vector<T>::emplace_back(Args&&... args)
    {
        if(m_Size >= m_Capacity) grow(m_Capacity + 1);

        m_Data[m_Size++] = T(std::forward<Args>(args)...);
    }

    template <typename T>
    T& vector<T>::push()
    {
        if(m_Size >= m_Capacity)
        {
            grow(m_Capacity + 1);
        }
        m_Size++;
        m_Data[m_Size - 1] = T();
        return back();
    }

    template <typename T>
    T& vector<T>::pop_back()
    {
        RAW_ASSERT_MSG(m_Size > 0, "vector's size is 0, unable to pop.");
        return m_Data[m_Size--];
    }

    template <typename T>
    void vector<T>::delete_swap(u32 index)
    {
        RAW_ASSERT(m_Size > 0 && index < m_Size);
        m_Data[index] = m_Data[--m_Size];
    }

    template <typename T>
    T& vector<T>::operator[](u32 index)
    {
        RAW_ASSERT_MSG(index < m_Size, "index %u exceeds vectors size of %u", index, m_Size);
        return m_Data[index];
    }

    template <typename T>
    const T& vector<T>::operator[](u32 index) const
    {
        RAW_ASSERT_MSG(index < m_Size, "index %u exceeds vectors size of %u", index, m_Size);
        return m_Data[index];
    }

    template <typename T>
    void vector<T>::clear()
    {
        m_Size = 0;
    }

    template <typename T>
    void vector<T>::reserve(u32 capacity)
    {
        if(capacity > m_Capacity) grow(capacity);
    }

    template <typename T>
    void vector<T>::resize(u32 size)
    {
        if(size > m_Capacity) grow(size);
        m_Size = size;
    }

    template <typename T>
    void vector<T>::grow(u32 capacity)
    {
        if(capacity < m_Capacity * 2)
        {
            capacity = m_Capacity * 2;
        }
        else if (capacity < 4)
        {
            capacity = 4;
        }

        // safe guards
        u32 curCap = m_Capacity;
        u32 curSize = m_Size;
        T* data = (T*)RAW_ALLOCATE(capacity * sizeof(T), alignof(T));
        if(curCap)
        {
            MemoryCopy(data, m_Data, m_Capacity * sizeof(T));
            RAW_DEALLOCATE(m_Data);
        }

        memset(data + curSize, 0, (capacity - m_Capacity) * sizeof(T));

        m_Data = data;
        m_Size = curSize;
        m_Capacity = capacity;
    }

    template <typename T>
    u32 vector<T>::size() const
    {
        return m_Size;
    }

    template <typename T>
    u32 vector<T>::capacity() const
    {
        return m_Capacity;
    }

    template <typename T>
    T* vector<T>::data() const 
    {
        return m_Data;
    }

    template <typename T>
    T& vector<T>::back()
    {
        RAW_ASSERT(m_Size > 0);
        return m_Data[m_Size - 1];
    }

    template <typename T>
    T& vector<T>::front()
    {
        RAW_ASSERT(m_Size > 0);
        return m_Data[0];
    }

    template <typename T>
    const T& vector<T>::front() const
    {
        RAW_ASSERT(m_Size > 0);
        return m_Data[0];
    }
}