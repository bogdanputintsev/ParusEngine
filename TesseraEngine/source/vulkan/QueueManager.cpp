#include "QueueManager.h"

#include <vector>

#include "CommandBufferManager.h"
#include "DeviceManager.h"
#include "SurfaceManager.h"
#include "SwapChainManager.h"
#include "SyncObjectsManager.h"
#include "utils/interfaces/ServiceLocator.h"

namespace tessera::vulkan
{

	void QueueManager::init()
	{
		Initializable::init();
		const auto& logicalDevice = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();
		const auto& physicalDevice = ServiceLocator::getService<DeviceManager>()->getPhysicalDevice();
		const auto& surface = ServiceLocator::getService<SurfaceManager>()->getSurface();

		const auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice, surface);

		vkGetDeviceQueue(logicalDevice, graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, presentFamily.value(), 0, &presentQueue);
	}

	void QueueManager::drawFrame() const
	{
		const auto& syncObjectsManager = ServiceLocator::getService<SyncObjectsManager>();
		const auto& imageAvailableSemaphore = syncObjectsManager->getImageAvailableSemaphore();
		const auto& renderFinishedSemaphore = syncObjectsManager->getRenderFinishedSemaphore();
		const auto& inFlightFence = syncObjectsManager->getInFlightFence();
		const auto& swapChainManager = ServiceLocator::getService<SwapChainManager>();
		const auto& swapChain = swapChainManager->getSwapChain();
		const auto& commandBufferManager = ServiceLocator::getService<CommandBufferManager>();
		const auto& commandBuffer = commandBufferManager->getCommandBuffer();

		syncObjectsManager->waitForFences();
		const uint32_t imageIndex = swapChainManager->acquireNextImage();
		commandBufferManager->resetCommandBuffer();
		commandBufferManager->recordCommandBuffer(commandBuffer, imageIndex);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		const VkSemaphore waitSemaphores[] = { imageAvailableSemaphore };
		constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		const VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence) != VK_SUCCESS)
		{
			throw std::runtime_error("GlfwInitializer: failed to submit draw command buffer.");
		}

		// Submit result back to swap chain to have it eventually show up on the screen. 
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		const VkSwapchainKHR swapChains[] = { swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr;
		vkQueuePresentKHR(presentQueue, &presentInfo);
	}

	QueueFamilyIndices findQueueFamilies(const VkPhysicalDevice& physicalDevice, const VkSurfaceKHR& surface)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

		for (uint32_t i = 0; i < queueFamilyCount; ++i)
		{
			if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
			}

			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
			if (presentSupport)
			{
				indices.presentFamily = i;
			}

			if (indices.isComplete())
			{
				break;
			}
		}

		return indices;
	}

	bool QueueFamilyIndices::isComplete() const
	{
		return graphicsFamily.has_value() && presentFamily.has_value();
	}

}
