#pragma once
#include <string>

#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    
    class VkRenderPassBuilder final
    {
    public:
        static void build(const std::string& name, VulkanStorage& storage);
    };
    
}
