#include "SyncObjectsManager.h"

#include "CommandBufferManager.h"
#include "utils/interfaces/ServiceLocator.h"
#include "vulkan/DeviceManager.h"

namespace tessera::vulkan
{

	void SyncObjectsManager::init()
	{
		Initializable::init();

		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();
		const auto& commandBufferManager = ServiceLocator::getService<CommandBufferManager>();

		const auto numberOfBuffers = commandBufferManager->getNumberOfBuffers();

		imageAvailableSemaphores.resize(numberOfBuffers);
		renderFinishedSemaphores.resize(numberOfBuffers);
		inFlightFences.resize(numberOfBuffers);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i = 0; i < numberOfBuffers; ++i)
		{
			if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
				vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
				vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS)
			{
				throw std::runtime_error("SyncObjectsManager: failed to create semaphores.");
			}
		}
	}

	void SyncObjectsManager::clean()
	{
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		for (size_t i = 0; i < imageAvailableSemaphores.size(); ++i)
		{
			vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(device, inFlightFences[i], nullptr);
		}
		
	}

	void SyncObjectsManager::waitForFences(const int currentFrame) const
	{
		if (static_cast<size_t>(currentFrame) >= inFlightFences.size() || currentFrame < 0)
		{
			throw std::out_of_range("SyncObjectsManager: current frame number is larger than number of fences.");
		}

		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(device, 1, &inFlightFences[currentFrame]);
	}
}
