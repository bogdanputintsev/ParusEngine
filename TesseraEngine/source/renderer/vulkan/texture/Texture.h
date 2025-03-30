#pragma once
#include <vulkan/vulkan_core.h>

    
namespace tessera
{
    struct Texture
    {
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;
        uint32_t maxMipLevels;
    };
    
}
