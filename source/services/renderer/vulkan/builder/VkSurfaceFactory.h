#pragma once

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkSurfaceFactory
    {
    public:
        static void build(VulkanStorage& vulkanStorage);
    };
}   