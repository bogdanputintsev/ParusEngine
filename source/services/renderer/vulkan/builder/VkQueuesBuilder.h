#pragma once


#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    class VkQueuesBuilder
    {
    public:
        static void build(VulkanStorage& storage);
    };
}
