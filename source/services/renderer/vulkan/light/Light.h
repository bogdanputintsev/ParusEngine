#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "services/world/SceneLight.h"

namespace parus::vulkan
{

    struct VulkanPointLight final : public parus::PointLight
    {
        VulkanPointLight() = default;
        explicit VulkanPointLight(const parus::PointLight& light) : parus::PointLight(light) {}
    };

    struct VulkanDirectionalLight final : public parus::DirectionalLight
    {
        VulkanDirectionalLight() = default;
        explicit VulkanDirectionalLight(const parus::DirectionalLight& light) : parus::DirectionalLight(light) {}

        std::vector<VkDescriptorSet> descriptorSets;
    };

}
