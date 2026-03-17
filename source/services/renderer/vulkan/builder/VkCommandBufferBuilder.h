#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkCommandBufferBuilder final
    {
    public:
        [[nodiscard]] std::vector<VkCommandBuffer> build(const std::string& name, const VulkanStorage& storage) const;

        VkCommandBufferBuilder& setCommandPool(VkCommandPool newCommandPool);
        VkCommandBufferBuilder& setCount(uint32_t newCount);

    private:
        VkCommandPool commandPool = VK_NULL_HANDLE;
        uint32_t count = 1;
    };
}