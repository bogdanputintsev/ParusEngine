#pragma once

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkSyncObjectsFactory final
    {
    public:
        void build(VulkanStorage& storage) const;
    };
}