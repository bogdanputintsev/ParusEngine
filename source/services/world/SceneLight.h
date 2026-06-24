#pragma once
#include "engine/utils/math/Math.h"

namespace parus
{

    struct DirectionalLight
    {
        /** Light emission color (RGB). */
        math::Vector3 color;
        /** Normalized direction vector. */
        math::Vector3 direction;
    };

    struct PointLight
    {
        /** Light position in world space. */
        math::Vector3 position;
        /** Light emission color (RGB). */
        math::Vector3 color;
        /** Radius of light influence. */
        float radius = 50.0f;
        /** Light intensity multiplier. */
        float intensity = 1.0f;
    };

}
