#pragma once

#include <string>
#include <vulkan/vulkan_core.h>
#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkCommandPoolFactory final
    {
    public:
        [[nodiscard]] VkCommandPool build(const std::string& name, const VulkanStorage& storage) const;
    };
}