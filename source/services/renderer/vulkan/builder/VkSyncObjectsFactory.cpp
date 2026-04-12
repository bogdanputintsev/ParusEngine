#include "VkSyncObjectsFactory.h"

#include "services/renderer/vulkan/utils/VulkanUtils.h"

namespace parus::vulkan
{
    void VkSyncObjectsFactory::build(VulkanStorage& storage) const
    {
        storage.imageAvailableSemaphores.resize(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
        storage.renderFinishedSemaphores.resize(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
        storage.inFlightFences.resize(VulkanStorage::MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (int i = 0; i < VulkanStorage::MAX_FRAMES_IN_FLIGHT; ++i)
        {
            ASSERT(vkCreateSemaphore(storage.logicalDevice, &semaphoreInfo, nullptr, &storage.imageAvailableSemaphores[i]) == VK_SUCCESS,
                "Failed to create image available semaphore.");
            utils::setDebugName(storage, storage.imageAvailableSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE,
                ("Image Available Semaphore [" + std::to_string(i) + "]").c_str());

            ASSERT(vkCreateSemaphore(storage.logicalDevice, &semaphoreInfo, nullptr, &storage.renderFinishedSemaphores[i]) == VK_SUCCESS,
                "Failed to create render finished semaphore.");
            utils::setDebugName(storage, storage.renderFinishedSemaphores[i], VK_OBJECT_TYPE_SEMAPHORE,
                ("Render Finished Semaphore [" + std::to_string(i) + "]").c_str());

            ASSERT(vkCreateFence(storage.logicalDevice, &fenceInfo, nullptr, &storage.inFlightFences[i]) == VK_SUCCESS,
                "Failed to create in-flight fence.");
            utils::setDebugName(storage, storage.inFlightFences[i], VK_OBJECT_TYPE_FENCE,
                ("In-Flight Fence [" + std::to_string(i) + "]").c_str());
        }
    }
}