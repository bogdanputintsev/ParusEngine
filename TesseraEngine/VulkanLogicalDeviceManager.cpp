#include "VulkanLogicalDeviceManager.h"

#include <cassert>
#include <set>
#include <stdexcept>

#include "VulkanDebugManager.h"
#include "VulkanQueueFamiliesManager.h"

namespace tessera::vulkan
{
	void VulkanLogicalDeviceManager::init(const std::shared_ptr<const VkInstance>& instance, const std::shared_ptr<const VkSurfaceKHR>& surface)
	{
		const VkPhysicalDevice physicalDevice = VulkanPhysicalDeviceManager::pickAnySuitableDevice(*instance, surface);
		assert(physicalDevice);

		const auto [graphicsFamily, presentFamily] = VulkanQueueFamiliesManager::findQueueFamilies(physicalDevice, surface);
		std::set uniqueQueueFamilies = { graphicsFamily.value(), presentFamily.value() };

		constexpr float queuePriority = 1.0f;
		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

		for (uint32_t queueFamily : uniqueQueueFamilies) 
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.emplace_back(queueCreateInfo);
		}

		// Specifying used device features.
		constexpr VkPhysicalDeviceFeatures deviceFeatures{};

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
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
		vkGetDeviceQueue(device, presentFamily.value(), 0, &presentQueue);
	}

	void VulkanLogicalDeviceManager::clean() const
	{
		vkDestroyDevice(device, nullptr);
	}

}
