#pragma once

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkImageViewBuilder final
    {
    public:
        [[nodiscard]] VkImageView build(const std::string& name, const VulkanStorage& vulkanStorage) const;

        VkImageViewBuilder& setImage(const VkImage& newImage);
        VkImageViewBuilder& setViewType(const VkImageViewType newViewType);
        VkImageViewBuilder& setFormat(const VkFormat& newFormat);
        VkImageViewBuilder& setComponents(const VkComponentMapping& newComponents);
        VkImageViewBuilder& setAspectMask(const VkImageAspectFlags& newAspectMask);
        VkImageViewBuilder& setLevelCount(const uint32_t newLevelCount);
        VkImageViewBuilder& setLayerCount(const uint32_t newLayerCount);
    private:
        VkImage image = VK_NULL_HANDLE;
        VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkComponentMapping components = {};
        VkImageAspectFlags aspectMask = 0;
        uint32_t levelCount = 1;
        uint32_t layerCount = 1;
    };
}
