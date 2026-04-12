#pragma once


#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    class VkQueuesFactory
    {
    public:
        static void build(VulkanStorage& storage);
    };
}
