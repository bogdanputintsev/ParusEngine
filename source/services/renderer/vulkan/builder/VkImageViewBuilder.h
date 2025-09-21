#pragma once

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkImageViewBuilder
    {
    public:
        [[nodiscard]] VkImageView build(const std::string& name, const VulkanStorage& vulkanStorage) const;

        VkImageViewBuilder& setImage(const VkImage& newImage);
        VkImageViewBuilder& setFormat(const VkFormat& newFormat);
        VkImageViewBuilder& setAspectFlags(const VkImageAspectFlags& newAspectFlags);
        VkImageViewBuilder& setMipLevels(const uint32_t newMipLevels);
        
    private:
        VkImage image = VK_NULL_HANDLE;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageAspectFlags aspectFlags = 0;
        uint32_t mipLevels = 0;
    };
}
