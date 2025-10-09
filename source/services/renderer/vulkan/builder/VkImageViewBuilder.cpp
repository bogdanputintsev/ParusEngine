#include "VkImageViewBuilder.h"


namespace parus::vulkan
{
    VkImageView VkImageViewBuilder::build(const std::string& name, const VulkanStorage& vulkanStorage) const
    {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = viewType;
        viewInfo.format = format;
        viewInfo.components = components;
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = levelCount;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = layerCount;

        VkImageView imageView;
        ASSERT(vkCreateImageView(vulkanStorage.logicalDevice, &viewInfo, nullptr, &imageView) == VK_SUCCESS, "Failed to create texture image view.");

        utils::setDebugName(vulkanStorage, imageView, VK_OBJECT_TYPE_IMAGE_VIEW, name.c_str());
        
        return imageView;
    }

    VkImageViewBuilder& VkImageViewBuilder::setImage(const VkImage& newImage)
    {
        image = newImage;
        return *this;
    }

    VkImageViewBuilder& VkImageViewBuilder::setViewType(const VkImageViewType newViewType)
    {
        viewType = newViewType;
        return *this;
    }

    VkImageViewBuilder& VkImageViewBuilder::setFormat(const VkFormat& newFormat)
    {
        format = newFormat;
        return *this;
    }

    VkImageViewBuilder& VkImageViewBuilder::setComponents(const VkComponentMapping& newComponents)
    {
        components = newComponents;
        return *this;
    }

    VkImageViewBuilder& VkImageViewBuilder::setAspectMask(const VkImageAspectFlags& newAspectMask)
    {
        aspectMask = newAspectMask;
        return *this;
    }

    VkImageViewBuilder& VkImageViewBuilder::setLevelCount(const uint32_t newLevelCount)
    {
        levelCount = newLevelCount;
        return *this;
    }

    VkImageViewBuilder& VkImageViewBuilder::setLayerCount(const uint32_t newLayerCount)
    {
        layerCount = newLayerCount;
        return *this;
    }
}
