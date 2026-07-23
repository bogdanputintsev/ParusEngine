#pragma once
#include <cstdint>
#include <string>

#include "engine/utils/math/Math.h"

namespace parus
{

    /** Whether an entity can be moved and ticked, or is fixed for the lifetime of the scene. */
    enum class Mobility : uint8_t
    {
        Static,
        Movable
    };

    using EntityId = uint32_t;

    /** A named object in the world: a transform plus whatever optional components EntityManager has attached to its id. */
    struct Entity final
    {
        EntityId id = 0;
        std::string name;
        Mobility mobility = Mobility::Static;
        math::Matrix4x4 transform = math::Matrix4x4::identity();
    };

}
