#pragma once

#include "engine/utils/math/Math.h"
#include "services/renderer/vulkan/texture/VulkanTexture2d.h"
#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    
    class VulkanTexture2dBuilder final
    {
    public:
        explicit VulkanTexture2dBuilder(std::string imageName);
        
        [[nodiscard]] VulkanTexture2d build(const VulkanStorage& storage) const;
        [[nodiscard]] VulkanTexture2d buildFromFile(const std::string& filePath);
        [[nodiscard]] VulkanTexture2d buildFromSolidColor(const math::Vector3& color);
        
        VulkanTexture2dBuilder& setImageSize(const uint32_t newWidth, const uint32_t newHeight);
        VulkanTexture2dBuilder& setWidth(const uint32_t newWidth);
        VulkanTexture2dBuilder& setHeight(const uint32_t newHeight);
        VulkanTexture2dBuilder& setNumberOfMipLevels(const uint32_t newNumberOfMipLevels);
        VulkanTexture2dBuilder& setNumberOfSamples(const VkSampleCountFlagBits newNumberOfSamples);
        VulkanTexture2dBuilder& setFormat(const VkFormat newFormat);
        VulkanTexture2dBuilder& setAspectMask(const VkImageAspectFlagBits newAspectMask);
        VulkanTexture2dBuilder& setTiling(const VkImageTiling newTiling);
        VulkanTexture2dBuilder& setUsage(const VkImageUsageFlags newUsage);
        VulkanTexture2dBuilder& setPropertyFlags(const VkMemoryPropertyFlags newPropertyFlags);
        
    private:
        std::string debugName;

        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t numberOfMipLevels = 1;
        VkSampleCountFlagBits numberOfSamples = VK_SAMPLE_COUNT_1_BIT;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageAspectFlagBits aspectMask = VK_IMAGE_ASPECT_NONE;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VkMemoryPropertyFlags propertyFlags = 0;
    };
}
