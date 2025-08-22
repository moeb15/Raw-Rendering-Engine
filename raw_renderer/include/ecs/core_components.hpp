#pragma once

#include "ecs/component.hpp"
#include <limits>
#include <glm/glm.hpp>

namespace Raw
{
    struct TransformComponent : public IComponent
    {
        glm::vec3 position{ 0.0f };
        glm::vec3 rotation{ 0.0f };
        glm::vec3 scale{ 1.f };
    };

    struct HierarchyComponent : public IComponent
    {
        Entity parentId{ INVALID_ENTITY_ID };
        glm::mat4 parentWorldInv{ 1.f };
    };
}