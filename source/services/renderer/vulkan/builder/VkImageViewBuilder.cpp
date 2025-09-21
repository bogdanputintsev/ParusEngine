#include "VkImageViewBuilder.h"


namespace parus::vulkan
{
    VkImageView VkImageViewBuilder::build(const VulkanStorage& vulkanStorage) const
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        VkImageView imageView;
        ASSERT(vkCreateImageView(vulkanStorage.logicalDevice, &viewInfo, nullptr, &imageView) == VK_SUCCESS, "Failed to create texture image view.");

        return imageView;
    }

    VkImageViewBuilder& VkImageViewBuilder::setImage(const VkImage& newImage)
    {
        image = newImage;
        return *this;
    }

    VkImageViewBuilder& VkImageViewBuilder::setFormat(const VkFormat& newFormat)
    {
        format = newFormat;
        return *this;
    }

    VkImageViewBuilder& VkImageViewBuilder::setAspectFlags(const VkImageAspectFlags& newAspectFlags)
    {
        aspectFlags = newAspectFlags;
        return *this;
    }

    VkImageViewBuilder& VkImageViewBuilder::setMipLevels(const uint32_t newMipLevels)
    {
        mipLevels = newMipLevels;
        return *this;
    }
}
