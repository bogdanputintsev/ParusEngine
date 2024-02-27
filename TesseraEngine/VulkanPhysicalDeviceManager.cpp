#include "VulkanPhysicalDeviceManager.h"

#include <stdexcept>
#include <vector>

namespace tessera::vulkan
{
	
	void VulkanPhysicalDeviceManager::pickAnySuitableDevice(const VkInstance& instance)
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
				physicalDevice = device;
				break;
			}
		}

		if (physicalDevice == VK_NULL_HANDLE) 
		{
			throw std::runtime_error("VulkanPhysicalDeviceManager: failed to find a suitable GPU!");
		}
	}

	bool VulkanPhysicalDeviceManager::isDeviceSuitable(const VkPhysicalDevice& device)
	{
		// Basic device properties like the name, type and supported Vulkan version.
		VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		// The support for optional features like texture compression, 64 bit floats and multi viewport rendering.
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		// Now we don't need any specific requirements, and any GPU can be used.
		return true;
	}

}
