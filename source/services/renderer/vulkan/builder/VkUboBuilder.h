#pragma once

#include <string>
#include <vulkan/vulkan_core.h>
#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkUboBuilder final
    {
    public:
        [[nodiscard]] UboBuffer build(const std::string& name, const VulkanStorage& storage) const;

        VkUboBuilder& setSize(VkDeviceSize newSize);

    private:
        VkDeviceSize size = 0;
    };
}