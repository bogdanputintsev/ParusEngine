#pragma once
#include <memory>

#include "engine/utils/math/Math.h"
#include "services/renderer/vulkan/mesh/Mesh.h"

namespace parus
{

    struct WorldMeshInstance
    {
        std::shared_ptr<Mesh> mesh;
        math::Matrix4x4 transform = math::Matrix4x4::identity();
    };

}
