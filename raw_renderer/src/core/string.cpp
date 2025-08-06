#include "core/string.hpp"
#include "core/logger.hpp"

namespace Raw::rstd
{
    string::string()
    {
        m_Str = (char*)RAW_ALLOCATE(sizeof(char), alignof(char));
        m_Str[0] = '\0';
        m_Len = 0;
    }

    void string::clear()
    {
        RAW_DEALLOCATE(m_Str);
        m_Len = 0;
        m_Str = nullptr;
    }

    string::~string()
    {
        clear();
    }
    
    string::string(cstring val)
    {
        if(val == nullptr)
        {
            m_Str = (char*)RAW_ALLOCATE(sizeof(char), alignof(char));
            m_Str[0] = '\0';
            m_Len = 0;
        }
        else
        {
            m_Len = (u32)strlen(val);
            m_Str = (char*)RAW_ALLOCATE(sizeof(char) * (m_Len + 1), alignof(char));
            strcpy(m_Str, val);
            m_Str[m_Len] = '\0';
        }
    }
}