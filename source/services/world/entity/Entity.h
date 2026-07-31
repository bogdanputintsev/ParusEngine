#pragma once
#include <cstdint>
#include <string>

#include "engine/utils/math/Math.h"
#include "services/world/entity/Components.h"

namespace parus
{

    using EntityId = uint32_t;

    /** A named object in the world: a transform plus whatever optional components EntityManager has attached to its id. */
    struct Entity final
    {
        EntityId id = 0;
        std::string name;
        Mobility mobility = Mobility::Static;
        math::Transform transform;
    };

}
