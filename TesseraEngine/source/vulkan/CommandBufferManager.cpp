#include "CommandBufferManager.h"

#include <stdexcept>

#include "QueueFamiliesManager.h"

namespace tessera::vulkan
{
	
	void CommandBufferManager::init(const std::shared_ptr<const VkDevice>& device, const std::shared_ptr<const VkPhysicalDevice>& physicalDevice, const std::shared_ptr<const VkSurfaceKHR>& surface)
	{
		const auto [graphicsFamily, presentFamily] = QueueFamiliesManager::findQueueFamilies(*physicalDevice, surface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = graphicsFamily.value();

		if (vkCreateCommandPool(*device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) 
		{
			throw std::runtime_error("CommandPoolManager: failed to create command pool.");
		}
	}

	void CommandBufferManager::clean(const std::shared_ptr<const VkDevice>& device) const
	{
		vkDestroyCommandPool(*device, commandPool, nullptr);
	}

}
