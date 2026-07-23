#pragma once
#include <memory>

#include "engine/utils/math/Math.h"
#include "services/renderer/vulkan/mesh/Mesh.h"

namespace parus
{

    /** Attaches renderable geometry to an entity. */
    struct MeshComponent final
    {
        std::shared_ptr<Mesh> mesh;
    };

    /**
     * A local light source with a position inherited from the owning entity's transform.
     * Not final: the renderer's VulkanPointLight extends it with GPU-side state.
     */
    struct PointLightComponent
    {
        /** Light emission color (RGB). */
        math::Vector3 color;
        /** Radius of light influence. */
        float radius = 50.0f;
        /** Light intensity multiplier. */
        float intensity = 1.0f;
    };

    /**
     * The scene's sun. EntityManager enforces at most one entity holds this component.
     * Not final: the renderer's VulkanDirectionalLight extends it with GPU-side state.
     */
    struct DirectionalLightComponent
    {
        /** Light emission color (RGB). */
        math::Vector3 color;
        /** Normalized direction vector. */
        math::Vector3 direction;
    };

    /** The scene's sky. EntityManager enforces at most one entity holds this component. */
    struct SkyboxComponent final
    {
        std::shared_ptr<Mesh> mesh;
        math::Vector3 horizonColor;
        math::Vector3 zenithColor;
    };

}
