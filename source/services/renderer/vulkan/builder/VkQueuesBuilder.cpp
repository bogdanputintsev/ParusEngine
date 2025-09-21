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

        std::lock_guard lock(storage.graphicsQueueMutex);
        {
            vkGetDeviceQueue(storage.logicalDevice, graphicsFamily.value(), 0, &storage.graphicsQueue);
            ASSERT(storage.graphicsQueue != VK_NULL_HANDLE, "Graphics Queue has not been created.");
            utils::setDebugName(storage, storage.graphicsQueue, VK_OBJECT_TYPE_QUEUE, "Graphics Queue");
            
            vkGetDeviceQueue(storage.logicalDevice, presentFamily.value(), 0, &storage.presentQueue);
            ASSERT(storage.presentQueue != VK_NULL_HANDLE, "Present Queue has not been created.");
            utils::setDebugName(storage, storage.presentQueue, VK_OBJECT_TYPE_QUEUE, "Present Queue");
        }
        
    }
    
}
