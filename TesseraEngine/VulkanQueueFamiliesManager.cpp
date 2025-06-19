#include "VulkanQueueFamiliesManager.h"

#include <vector>

namespace tessera::vulkan
{

	QueueFamilyIndices VulkanQueueFamiliesManager::findQueueFamilies(const VkPhysicalDevice& physicalDevice)
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

			if (indices.isComplete())
			{
				break;
			}
		}

		return indices;
	}

	bool QueueFamilyIndices::isComplete() const
	{
		return graphicsFamily.has_value();
	}

}
