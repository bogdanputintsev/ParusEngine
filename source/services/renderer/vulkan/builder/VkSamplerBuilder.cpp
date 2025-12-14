#include "VkSamplerBuilder.h"


namespace parus::vulkan
{
    VkSamplerBuilder::VkSamplerBuilder(std::string newDebugName)
        : debugName(std::move(newDebugName))
    {
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 0.0f;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.mipLodBias = 0.0f;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = 1.0f;
    }

    VkSampler VkSamplerBuilder::build(const VulkanStorage& storage) const
    {
        VkSampler cubemapSampler;
        ASSERT(vkCreateSampler(storage.logicalDevice, &samplerInfo, nullptr, &cubemapSampler) == VK_SUCCESS, 
            "Failed to create cubemap sampler " + debugName);
        
        utils::setDebugName(storage, cubemapSampler, VK_OBJECT_TYPE_SAMPLER, debugName.c_str());
        
        return cubemapSampler;
    }

    VkSamplerBuilder& VkSamplerBuilder::setSamplerMode(
        const VkSamplerAddressMode newAddressModeU,
        const VkSamplerAddressMode newAddressModeV, 
        const VkSamplerAddressMode newAddressModeW)
    {
        samplerInfo.addressModeU = newAddressModeU;
        samplerInfo.addressModeV = newAddressModeV;
        samplerInfo.addressModeW = newAddressModeW;
        return *this;
    }

    VkSamplerBuilder& VkSamplerBuilder::setSamplerMode(const VkSamplerAddressMode newAddressMode)
    {
        samplerInfo.addressModeU = newAddressMode;
        samplerInfo.addressModeV = newAddressMode;
        samplerInfo.addressModeW = newAddressMode;
        return *this;
    }

    VkSamplerBuilder& VkSamplerBuilder::setBorderColor(const VkBorderColor newBorderColor)
    {
        samplerInfo.borderColor = newBorderColor;
        return *this;
    }

    VkSamplerBuilder& VkSamplerBuilder::setMaxAnisotropy(const float newMaxAnisotropy)
    {
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = newMaxAnisotropy;
        return *this;
    }

    VkSamplerBuilder& VkSamplerBuilder::setMaxLod(const float newMaxLod)
    {
        samplerInfo.maxLod = newMaxLod;
        return *this;
    }
}
