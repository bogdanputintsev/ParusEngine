#pragma once
#include <optional>
#include <string>
#include <vector>

#include "engine/utils/math/Math.h"
#include "services/world/entity/Entity.h"

namespace parus::serialization
{

    struct EntityMeshEntry
    {
        uint32_t meshIndex = 0;
    };

    struct EntityPointLightEntry
    {
        math::Vector3 color;
        float radius = 50.0f;
        float intensity = 1.0f;
    };

    /** One row of the entity table: every entity's core fields, plus whichever optional components it has. */
    struct EntityEntry
    {
        std::string name;
        parus::Mobility mobility = parus::Mobility::Static;
        math::Matrix4x4 transform;

        std::optional<EntityMeshEntry> meshComponent;
        std::optional<EntityPointLightEntry> pointLightComponent;
    };

    struct DirectionalLightEntry
    {
        math::Vector3 color;
        math::Vector3 direction;
    };

    struct SkyboxEntry
    {
        std::string meshStem;
        math::Vector3 horizonColor;
        math::Vector3 zenithColor;
    };

    /** Parsed contents of a .pworld file - no Vulkan types. */
    struct SceneData
    {
        math::Vector3 cameraPosition;
        float         cameraYaw   = 0.0f;
        float         cameraPitch = 0.0f;

        DirectionalLightEntry directionalLight;
        SkyboxEntry           skybox;

        /** Ordered list of mesh stems; EntityMeshEntry.meshIndex indexes this. */
        std::vector<std::string> meshStems;
        std::vector<EntityEntry> entities;
    };

}
