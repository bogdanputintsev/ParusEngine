#include "VkQueuesBuilder.h"

#include <mutex>

#include "engine/EngineCore.h"
#include "services/renderer/vulkan/utils/VulkanUtils.h"

namespace parus::vulkan
{
    
    void VkQueuesBuilder::build(VulkanStorage& storage)
    {
        const auto [graphicsFamily, presentFamily] = utils::findQueueFamilies(storage.physicalDevice, storage.surface);

        ASSERT(graphicsFamily.has_value() && presentFamily.has_value(), "Queue family is undefined.");

        std::lock_guard<std::mutex> lock(storage.graphicsQueueMutex);
        vkGetDeviceQueue(storage.logicalDevice, graphicsFamily.value(), 0, &storage.graphicsQueue);
        vkGetDeviceQueue(storage.logicalDevice, presentFamily.value(), 0, &storage.presentQueue);
    }
    
}
