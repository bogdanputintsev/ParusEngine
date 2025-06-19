#include "SyncObjectsManager.h"

#include "utils/interfaces/ServiceLocator.h"
#include "vulkan/DeviceManager.h"

namespace tessera::vulkan
{

	void SyncObjectsManager::init()
	{
		Initializable::init();

		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkSemaphore imageAvailableSemaphoreInstance;
		VkSemaphore renderFinishedSemaphoreInstance;
		VkFence inFlightFenceInstance;

		if (vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &imageAvailableSemaphoreInstance) != VK_SUCCESS ||
			vkCreateSemaphore(*device, &semaphoreInfo, nullptr, &renderFinishedSemaphoreInstance) != VK_SUCCESS ||
			vkCreateFence(*device, &fenceInfo, nullptr, &inFlightFenceInstance) != VK_SUCCESS) 
		{
			throw std::runtime_error("SyncObjectsManager: failed to create semaphores.");
		}

		imageAvailableSemaphore = std::make_shared<VkSemaphore>(imageAvailableSemaphoreInstance);
		renderFinishedSemaphore = std::make_shared<VkSemaphore>(renderFinishedSemaphoreInstance);
		inFlightFence = std::make_shared<VkFence>(inFlightFenceInstance);
	}

	void SyncObjectsManager::clean()
	{
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		vkDestroySemaphore(*device, *imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(*device, *renderFinishedSemaphore, nullptr);
		vkDestroyFence(*device, *inFlightFence, nullptr);
	}

	void SyncObjectsManager::waitForFences() const
	{
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		vkWaitForFences(*device, 1, &*inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(*device, 1, &*inFlightFence);
	}
}
