#pragma once

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkImageBuilder final
    {
    public:
        explicit VkImageBuilder(std::string name);
        [[nodiscard]] VkImage build(const VulkanStorage& storage) const;

        VkImageBuilder& setWidth(const uint32_t newWidth);
        VkImageBuilder& setHeight(const uint32_t newHeight);
        VkImageBuilder& setMipLevels(const uint32_t newMipLevels);
        VkImageBuilder& setArrayLayers(const uint32_t newArrayLayers);
        VkImageBuilder& setFormat(const VkFormat newFormat);
        VkImageBuilder& setTiling(const VkImageTiling newTiling);
        VkImageBuilder& setUsage(const VkImageUsageFlags newUsage);
        VkImageBuilder& setSamples(const VkSampleCountFlagBits newSamples);
        VkImageBuilder& setFlags(const VkImageCreateFlags newFlags);
    
    private:
        std::string debugName;
        uint32_t width = 0;
        uint32_t height = 0;
        uint32_t mipLevels = 1;
        uint32_t arrayLayers = 1;
        VkFormat format = VK_FORMAT_UNDEFINED;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
        VkImageCreateFlags flags = 0;
    };
}
