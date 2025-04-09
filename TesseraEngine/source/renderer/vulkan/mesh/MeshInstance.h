#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "math/Math.h"

struct Mesh;

namespace tessera
{
        
    struct MeshInstance
    {
        std::shared_ptr<Mesh> mesh;
        math::Matrix4x4 transform = math::Matrix4x4::identity();
        std::vector<VkDescriptorSet> instanceDescriptorSets;
    };
        
}
