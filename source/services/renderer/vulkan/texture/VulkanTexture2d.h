#pragma once

#include <vulkan/vulkan_core.h>


namespace parus::vulkan
{
    
    class VulkanTexture2d final
    {
    public:
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory imageMemory = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        uint32_t maxMipLevels = 0;
    };
}
