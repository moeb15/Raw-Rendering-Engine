#pragma once

#include "core/defines.hpp"

namespace Raw
{
    class IService
    {
    public:
        virtual void Init(void* config) {};
        virtual void Shutdown() {};
    };

    #define RAW_DECLARE_SERVICE(className) static className* Get();
}