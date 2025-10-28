#include "VkImageBuilder.h"


namespace parus::vulkan
{
    VkImageBuilder::VkImageBuilder(std::string name)
        : debugName(std::move(name))
    {
    }

    VkImage VkImageBuilder::build(const VulkanStorage& storage) const
    {
        ASSERT(width != 0 && height != 0, "Attempted to create image with invalid dimensions");
		
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = arrayLayers;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = samples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.flags = flags;
        
        VkImage newImage;
        ASSERT(vkCreateImage(storage.logicalDevice, &imageInfo, nullptr, &newImage) == VK_SUCCESS, "Failed to create image " + debugName);
        utils::setDebugName(storage, newImage, VK_OBJECT_TYPE_IMAGE, debugName.c_str());

        return newImage;
    }

    VkImageBuilder& VkImageBuilder::setWidth(const uint32_t newWidth)
    {
        width = newWidth;
        return *this;
    }

    VkImageBuilder& VkImageBuilder::setHeight(const uint32_t newHeight)
    {
        height = newHeight;
        return *this;
    }

    VkImageBuilder& VkImageBuilder::setMipLevels(const uint32_t newMipLevels)
    {
        mipLevels = newMipLevels;
        return *this;
    }

    VkImageBuilder& VkImageBuilder::setArrayLayers(const uint32_t newArrayLayers)
    {
        arrayLayers = newArrayLayers;
        return *this;
    }

    VkImageBuilder& VkImageBuilder::setFormat(const VkFormat newFormat)
    {
        format = newFormat;
        return *this;
    }

    VkImageBuilder& VkImageBuilder::setTiling(const VkImageTiling newTiling)
    {
        tiling = newTiling;
        return *this;
    }

    VkImageBuilder& VkImageBuilder::setUsage(const VkImageUsageFlags newUsage)
    {
        usage = newUsage;
        return *this;
    }

    VkImageBuilder& VkImageBuilder::setSamples(const VkSampleCountFlagBits newSamples)
    {
        samples = newSamples;
        return *this;
    }

    VkImageBuilder& VkImageBuilder::setFlags(const VkImageCreateFlags newFlags)
    {
        flags = newFlags;
        return *this;
    }
}
