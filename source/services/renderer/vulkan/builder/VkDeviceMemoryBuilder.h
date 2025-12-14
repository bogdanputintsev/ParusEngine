#pragma once

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkDeviceMemoryBuilder final
    {
    public:
        explicit VkDeviceMemoryBuilder(std::string name);
        [[nodiscard]] VkDeviceMemory build(const VulkanStorage& storage) const;

        VkDeviceMemoryBuilder& setImage(const VkImage& newImage);
        VkDeviceMemoryBuilder& setPropertyFlags(const VkMemoryPropertyFlags newPropertyFlags);
        
    private:
        std::string debugName;
        
        VkImage image = VK_NULL_HANDLE;
        VkMemoryPropertyFlags propertyFlags = 0; 
    };
}
