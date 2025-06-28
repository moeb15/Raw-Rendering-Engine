#pragma once

#include "core/defines.hpp"

namespace Raw
{
    class IEvent
    {
    public:
        virtual const u32 GetEventType() const = 0;
        bool m_Handled{ false };
    };
}