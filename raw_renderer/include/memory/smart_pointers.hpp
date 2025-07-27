#pragma once

#include "core/defines.hpp"
#include "core/asserts.hpp"
#include "memory/memory_service.hpp"
#include <utility>
#include <type_traits>

namespace Raw::rstd
{
    template <typename T>
    class unique_ptr
    {
    public:
        DISABLE_COPY(unique_ptr);
        
        explicit unique_ptr(T* ptr) : m_Ptr(ptr) {}
        unique_ptr() : m_Ptr(nullptr) {}
        ~unique_ptr() { reset(); }

        T* get() const;
        T* release();
        void reset(T* newPtr = nullptr);
        T* operator->() const;

        template <typename U = T>
        std::enable_if_t<!std::is_same_v<U, void>, U&> operator*() const
        {
            RAW_ASSERT(m_Ptr != nullptr);
            return m_Ptr;
        }

        template <typename U, typename = std::enable_if_t<std::is_base_of_v<T, U> || std::is_convertible_v<T*, U*>>>
        unique_ptr(unique_ptr<U>&& other) noexcept : m_Ptr(other.release()) {}

        template <typename U, typename = std::enable_if_t<std::is_base_of_v<T, U> || std::is_convertible_v<T*, U*>>>
        unique_ptr& operator=(unique_ptr&& other) noexcept
        {
            reset(other.release());
            return *this;
        }

    private:
        T* m_Ptr{ nullptr };

    };

    template <typename T, typename... Args>
    unique_ptr<T> make_unique(Args&&... args)
    {
        void* data = RAW_ALLOCATE(sizeof(T), alignof(T));
        T* newObj = new (data) T(std::forward<Args>(args)...);
        return unique_ptr<T>(newObj);
    }

    template <typename T>
    T* unique_ptr<T>::get() const
    {
        return m_Ptr;
    }
    
    template <typename T>
    T* unique_ptr<T>::release()
    {
        T* oldPtr = m_Ptr;
        m_Ptr = nullptr;
        return oldPtr;
    }
    
    template <typename T>
    void unique_ptr<T>::reset(T* newPtr)
    {
        T* oldPtr = release();
        m_Ptr = newPtr;
        RAW_DEALLOCATE((void*)oldPtr);
    }
    
    template <typename T>
    T* unique_ptr<T>::operator->() const
    {
        return get();
    }
}