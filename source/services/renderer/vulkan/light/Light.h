#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>

#include "services/world/entity/Components.h"

namespace parus::vulkan
{

    struct VulkanPointLight final : public parus::PointLightComponent
    {
        VulkanPointLight() = default;
        /** worldPosition comes from the owning entity's transform, since PointLightComponent itself has no position. */
        VulkanPointLight(const parus::PointLightComponent& light, const math::Vector3& worldPosition)
            : parus::PointLightComponent(light), position(worldPosition) {}

        math::Vector3 position;
    };

    struct VulkanDirectionalLight final : public parus::DirectionalLightComponent
    {
        VulkanDirectionalLight() = default;
        explicit VulkanDirectionalLight(const parus::DirectionalLightComponent& light) : parus::DirectionalLightComponent(light) {}

        std::vector<VkDescriptorSet> descriptorSets;
    };

}
