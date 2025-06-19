#include "VulkanPhysicalDeviceManager.h"

#include <stdexcept>
#include <vector>

#include "VulkanQueueFamiliesManager.h"

namespace tessera::vulkan
{
	VkPhysicalDevice VulkanPhysicalDeviceManager::pickAnySuitableDevice(const VkInstance& instance)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

		if (deviceCount == 0) 
		{
			throw std::runtime_error("VulkanPhysicalDeviceManager: failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

		for (const auto& device : devices) 
		{
			if (isDeviceSuitable(device)) 
			{
				return device;
			}
		}

		throw std::runtime_error("VulkanPhysicalDeviceManager: failed to find a suitable GPU!");
	}

	bool VulkanPhysicalDeviceManager::isDeviceSuitable(const VkPhysicalDevice& device)
	{
		// Basic device properties like the name, type and supported Vulkan version.
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		// The support for optional features like texture compression, 64 bit floats and multi viewport rendering.
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		// Check if device can process the commands we want to use.
		const QueueFamilyIndices indices = VulkanQueueFamiliesManager::findQueueFamilies(device);

		return indices.isComplete();
	}

}
