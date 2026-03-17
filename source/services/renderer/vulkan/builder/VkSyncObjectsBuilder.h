#pragma once

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkSyncObjectsBuilder final
    {
    public:
        void build(VulkanStorage& storage) const;
    };
}