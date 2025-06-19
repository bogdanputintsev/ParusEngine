#include "VulkanPhysicalDeviceManager.h"

#include <stdexcept>
#include <vector>

#include "VulkanExtensionManager.h"
#include "VulkanQueueFamiliesManager.h"
#include "VulkanSwapChainManager.h"

namespace tessera::vulkan
{
	void VulkanPhysicalDeviceManager::pickAnySuitableDevice(const std::shared_ptr<const VkInstance>& instance, const std::shared_ptr<const VkSurfaceKHR>& surface)
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(*instance, &deviceCount, nullptr);

		if (deviceCount == 0) 
		{
			throw std::runtime_error("VulkanPhysicalDeviceManager: failed to find GPUs with Vulkan support!");
		}

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(*instance, &deviceCount, devices.data());

		for (const auto& device : devices) 
		{
			if (isDeviceSuitable(device, surface)) 
			{
				physicalDevice = std::make_shared<VkPhysicalDevice>(device);
				return;
			}
		}

		throw std::runtime_error("VulkanPhysicalDeviceManager: failed to find a suitable GPU!");
	}

	bool VulkanPhysicalDeviceManager::isDeviceSuitable(const VkPhysicalDevice& device, const std::shared_ptr<const VkSurfaceKHR>& surface)
	{
		// Basic device properties like the name, type and supported Vulkan version.
		[[maybe_unused]] VkPhysicalDeviceProperties deviceProperties;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);

		// The support for optional features like texture compression, 64 bit floats and multi viewport rendering.
		[[maybe_unused]] VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		// Check if device can process the commands we want to use.
		const QueueFamilyIndices indices = VulkanQueueFamiliesManager::findQueueFamilies(device, surface);

		// Check if physical device supports swap chain extension.
		const bool extensionsSupported = VulkanExtensionManager::isDeviceExtensionSupported(device);

		// Check if physical device supports swap chain.
		const SwapChainSupportDetails swapChainSupport = VulkanSwapChainManager::querySwapChainSupport(device, surface);
		
		return indices.isComplete() && extensionsSupported && swapChainSupport.isComplete();
	}

}
