#pragma once

#include "core/defines.hpp"

namespace Raw
{
#define MAX_ENTITIES 1 << 16
#define INVALID_ENTITY_ID 0

    using Entity = u32;

    struct IComponent
    {
        virtual ~IComponent() {}
    };
}