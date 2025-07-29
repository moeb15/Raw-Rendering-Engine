#pragma once

#include "core/defines.hpp"
#include "core/asserts.hpp"
#include "memory/memory_service.hpp"
#include "memory/helpers.hpp"
#include <type_traits>

namespace Raw::rstd
{
    template <typename T>
    class list
    {
    private:
        struct node
        {
            template <typename... Args>
            node(Args&&... args) : data(std::forward<Args>(args)...) {}

            T data;
            node* next{ nullptr };
            node* prev{ nullptr };
        };

    public:
        class list_iterator
        {
        public:
            list_iterator(node* node) : m_Curr(node) {}
            bool is_valid() const { return m_Curr != nullptr; }
            T& operator*() noexcept
            {
                RAW_ASSERT(m_Curr != nullptr); 
                return m_Curr->data;
            }
            list_iterator& operator++() { m_Curr = m_Curr->next; return *this; }
            list_iterator operator++(int)
            {
                list_iterator tmp(*this);
                operator++();
                return tmp;
            }

            list_iterator& operator--() { m_Curr = m_Curr->prev; return *this; }
            list_iterator operator--(int)
            {
                list_iterator tmp(*this);
                operator--();
                return tmp;
            }

            bool operator!=(const list_iterator& other) const { return m_Curr != other.m_Curr; }
            
            T& operator->() const noexcept
            {
                RAW_ASSERT(m_Curr != nullptr);
                return m_Curr->data;
            }
            
        private:
            node* m_Curr{ nullptr };

        };

        DISABLE_COPY(list);

        list() {}
        ~list() { clear(); }

        void push_back(const T& item);
        void push_back(T&& item);
        
        template <typename... Args>
        void emplace_back(Args&&... args);
        
        void pop_front();
        void clear();
        list_iterator erase(const list_iterator& it);
        u32 size() const { return m_Size; }
        bool empty() const { return m_Size > 0; }
        T& back() const;
        T& front() const;

        node* get_tail() const { return m_Tail; }
        list_iterator begin() { return list_iterator(m_Head); }
        list_iterator end() { return list_iterator(nullptr); }

    private:
        node* m_Head{ nullptr };
        node* m_Tail{ nullptr };
        u32 m_Size{ 0 };

    };

    template <typename T>
    void list<T>::push_back(const T& item)
    {
        void* data = (node*)RAW_ALLOCATE(sizeof(node), alignof(node));
        node* newNode = new (data) node(item);
        node* tempPrev = m_Tail;

        if(!m_Head) m_Head = newNode;
        if(m_Tail) m_Tail->next = newNode;
        m_Tail = newNode;
        m_Tail->prev = tempPrev;
        tempPrev = nullptr;
        m_Size++;
    }

    template <typename T>
    void list<T>::push_back(T&& item)
    {
        void* data = (node*)RAW_ALLOCATE(sizeof(node), alignof(node));
        node* newNode = new (data) node(std::move(item));
        node* tempPrev = m_Tail;

        if(!m_Head) m_Head = newNode;
        if(m_Tail) m_Tail->next = newNode;
        m_Tail = newNode;
        m_Tail->prev = tempPrev;
        tempPrev = nullptr;
        m_Size++;
    }

    template <typename T>
    template <typename... Args>
    void list<T>::emplace_back(Args&&... args)
    {
        void* data = (node*)RAW_ALLOCATE(sizeof(node), alignof(node));
        node* newNode = new (data) node(std::forward<Args>(args)...);
        node* tempPrev = m_Tail;

        if(!m_Head) m_Head = newNode;
        if(m_Tail) m_Tail->next = newNode;
        m_Tail = newNode;
        m_Tail->prev = tempPrev;
        tempPrev = nullptr;
        m_Size++;
    }

    template <typename T>
    void list<T>::pop_front()
    {
        if(m_Head)
        {
            node* curr = m_Head;
            m_Head = m_Head->next;
            RAW_DEALLOCATE(curr);
            m_Size--;
        }
    }

    template <typename T>
    void list<T>::clear()
    {
        node* pCurr = m_Head;
        u32 curSize = 0;
        while(pCurr && curSize < m_Size)
        {
            node* next = pCurr->next;
            pCurr->data.~T();
            RAW_DEALLOCATE(pCurr);
            pCurr = next;
            curSize++;
        }

        m_Head = m_Tail = nullptr;
        m_Size = 0;
    }

    template <typename T>
    list<T>::list_iterator list<T>::erase(const list<T>::list_iterator& it)
    {
        it--;
        list_iterator& prev = it;
        it++;
        it++;
        list_iterator& next = it;
        
        prev.m_Curr->next = next.m_Curr;
        next.m_Curr->prev = prev.m_Curr;
        it--;
        it.m_Curr->next = nullptr;
        it.m_Curr->prev = nullptr;
        return it;
    }

    template <typename T>
    T& list<T>::back() const
    {
        RAW_ASSERT(m_Tail != nullptr);
        return m_Tail->data;
    }

    template <typename T>
    T& list<T>::front() const
    {
        RAW_ASSERT(m_Head != nullptr);
        return m_Head->data;
    }
}