#pragma once

#include <string>
#include <utility>
#include <vulkan/vulkan_core.h>
#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkBufferBuilder final
    {
    public:
        explicit VkBufferBuilder(std::string name);
        [[nodiscard]] std::pair<VkBuffer, VkDeviceMemory> build(const VulkanStorage& storage) const;

        VkBufferBuilder& setSize(VkDeviceSize newSize);
        VkBufferBuilder& setUsage(VkBufferUsageFlags newUsage);
        VkBufferBuilder& setMemoryProperties(VkMemoryPropertyFlags newMemoryProperties);

    private:
        std::string debugName;
        VkDeviceSize size = 0;
        VkBufferUsageFlags usage = 0;
        VkMemoryPropertyFlags memoryProperties = 0;
    };
}