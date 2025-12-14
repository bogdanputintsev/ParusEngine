#pragma once

#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    
    class VkSamplerBuilder final
    {
    public:
        explicit VkSamplerBuilder(std::string newDebugName);

        [[nodiscard]] VkSampler build(const VulkanStorage& storage) const;

        VkSamplerBuilder& setSamplerMode(
            const VkSamplerAddressMode newAddressModeU, 
            const VkSamplerAddressMode newAddressModeV, 
            const VkSamplerAddressMode newAddressModeW);
        VkSamplerBuilder& setSamplerMode(const VkSamplerAddressMode newAddressMode);
        VkSamplerBuilder& setBorderColor(const VkBorderColor newBorderColor);
        VkSamplerBuilder& setMaxAnisotropy(const float newMaxAnisotropy);
        VkSamplerBuilder& setMaxLod(const float newMaxLod);
        
    private:
        std::string debugName;
        
        VkSamplerCreateInfo samplerInfo{};
      
    };
}
