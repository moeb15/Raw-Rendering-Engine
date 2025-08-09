#pragma once

#include "core/defines.hpp"
#include "memory/memory_service.hpp"
#include <cstring>

namespace Raw::rstd
{
    class string
    {
    public:
        string();
        ~string();

        string(cstring val);
        
        RAW_INLINE string(const string& src)
        {
            m_Len = src.length();
            m_Str = (char*)RAW_ALLOCATE(sizeof(char) * (m_Len + 1), alignof(char));
            strcpy(m_Str, src.c_str());
            m_Str[m_Len] = '\0';
        }
        
        RAW_INLINE string& operator=(const string& src)
        {
            this->clear();
            this->m_Len = src.length();
            this->m_Str = (char*)RAW_ALLOCATE(sizeof(char) * (m_Len + 1), alignof(char));
            strcpy(m_Str, src.c_str());
            this->m_Str[m_Len] = '\0';
            return *this;
        }

        RAW_INLINE string(string&& src)
        {
            m_Str = src.m_Str;
            m_Len = src.m_Len;
            src.clear();
        }
        
        RAW_INLINE string& operator=(string&& src)
        {
            m_Str = src.m_Str;
            m_Len = src.m_Len;
            src.clear();
            return *this;
        }

        bool operator==(const string& val)
        {
            if(this->m_Len != val.m_Len) return false;
            return !strcmp(this->c_str(), val.c_str());
        }

        bool operator!=(const string& val)
        {
            if(this->m_Len != val.m_Len) return true;
            return strcmp(this->c_str(), val.c_str());
        }

        RAW_INLINE cstring c_str() const { return m_Str; }
        RAW_INLINE u32 length() const { return m_Len; }
    
    private:
        void clear();

    private:
        char* m_Str{ nullptr };
        u32 m_Len{ 0 };

    };
}