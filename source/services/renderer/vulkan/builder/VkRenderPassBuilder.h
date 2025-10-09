#pragma once
#include <string>

#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    
    class VkRenderPassBuilder
    {
    public:
        static void build(const std::string& name, VulkanStorage& storage);
    };
    
}
