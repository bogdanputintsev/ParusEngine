#pragma once
#include <string>
#include <vector>

#include "engine/utils/math/Math.h"
#include "services/world/SceneLight.h"

namespace parus::serialization
{

    struct MeshInstanceEntry
    {
        uint32_t        meshIndex = 0;
        math::Matrix4x4 transform;
    };

    /** Parsed contents of a .pworld file - no Vulkan types. */
    struct SceneData
    {
        math::Vector3 cameraPosition;
        float         cameraYaw   = 0.0f;
        float         cameraPitch = 0.0f;

        /** Reserved: stem of a custom sky mesh. Not acted on in iteration 2. */
        std::string   skyMeshStem;
        math::Vector3 skyHorizonColor;
        math::Vector3 skyZenithColor;

        DirectionalLight        directionalLight;
        std::vector<PointLight> pointLights;

        /** Ordered list of mesh stems; MeshInstanceEntry.meshIndex indexes this. */
        std::vector<std::string>       meshStems;
        std::vector<MeshInstanceEntry> meshInstances;
    };

}
