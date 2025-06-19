#include "VulkanLogicalDeviceManager.h"

#include <cassert>
#include <stdexcept>

#include "VulkanDebugManager.h"
#include "VulkanQueueFamiliesManager.h"

namespace tessera::vulkan
{

	void VulkanLogicalDeviceManager::init(const VkInstance& instance)
	{
		const VkPhysicalDevice physicalDevice = VulkanPhysicalDeviceManager::pickAnySuitableDevice(instance);
		assert(physicalDevice);

		const auto [graphicsFamily] = VulkanQueueFamiliesManager::findQueueFamilies(physicalDevice);

		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = graphicsFamily.value();
		queueCreateInfo.queueCount = 1;

		constexpr float queuePriority = 1.0f;
		queueCreateInfo.pQueuePriorities = &queuePriority;

		// Specifying used device features.
		constexpr VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = &queueCreateInfo;
		createInfo.queueCreateInfoCount = 1;
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = 0;

		// Distinction between instance and device specific validation no longer the case. This was added for back compatibility.
		const std::vector<const char*> validationLayers = VulkanDebugManager::getValidationLayers();
		if (VulkanDebugManager::validationLayersAreEnabled()) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else 
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanLogicalDeviceManager: failed to create logical device.");
		}

		vkGetDeviceQueue(device, graphicsFamily.value(), 0, &graphicsQueue);
	}

	void VulkanLogicalDeviceManager::clean() const
	{
		vkDestroyDevice(device, nullptr);
	}

}
