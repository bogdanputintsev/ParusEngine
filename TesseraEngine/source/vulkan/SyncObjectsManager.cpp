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

		if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphore) != VK_SUCCESS ||
			vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphore) != VK_SUCCESS ||
			vkCreateFence(device, &fenceInfo, nullptr, &inFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("SyncObjectsManager: failed to create semaphores.");
		}
	}

	void SyncObjectsManager::clean()
	{
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		vkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
		vkDestroyFence(device, inFlightFence, nullptr);
	}

	void SyncObjectsManager::waitForFences() const
	{
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &inFlightFence);
	}
}
