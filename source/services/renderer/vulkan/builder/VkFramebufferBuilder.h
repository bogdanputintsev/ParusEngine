#pragma once
#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    
    class VkFramebufferBuilder
    {
    public:
        void build();

        VkFramebufferBuilder& addColorImageView(VulkanStorage& storage);
        
    };
    
}
