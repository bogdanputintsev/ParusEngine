#pragma once
#include <string>

#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    
    class VkRenderPassFactory final
    {
    public:
        static void build(const std::string& name, VulkanStorage& storage);
    };
    
}
