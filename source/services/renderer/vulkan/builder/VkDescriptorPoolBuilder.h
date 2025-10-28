#pragma once
#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    
    class VkDescriptorPoolBuilder
    {
    public:
        void build(VulkanStorage& storage) const;

        VkDescriptorPoolBuilder& setMaxSets(const uint32_t newMaxSets);
        VkDescriptorPoolBuilder& setPoolSizes(const std::vector<VkDescriptorPoolSize>& newPoolSizes);

    private:
        uint32_t maxSets = 0;
        std::vector<VkDescriptorPoolSize> poolSizes;
        
    };
    
}
