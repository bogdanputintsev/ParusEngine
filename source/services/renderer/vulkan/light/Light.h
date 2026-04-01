#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "engine/utils/math/Math.h"
#include "engine/utils/math/UniformBufferObjects.h"

namespace parus::vulkan
{
    struct VulkanDirectionalLight
    {
        math::DirectionalLightUbo light;
        std::vector<VkDescriptorSet> descriptorSets;
    };

    struct PointLight
    {
        math::Vector3 position;
        math::Vector3 color;
        float radius = 50.0f;
        float intensity = 1.0f;
    };
}
