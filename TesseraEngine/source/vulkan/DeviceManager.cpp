#include "DeviceManager.h"

#include <cassert>
#include <set>
#include <stdexcept>

#include "DebugManager.h"
#include "ExtensionManager.h"
#include "InstanceManager.h"
#include "QueueManager.h"
#include "SurfaceManager.h"
#include "utils/interfaces/ServiceLocator.h"

namespace tessera::vulkan
{

	void DeviceManager::init()
	{
		const auto& instance = ServiceLocator::getService<InstanceManager>()->getInstance();
		const auto& surface = ServiceLocator::getService<SurfaceManager>()->getSurface();

		physicalDeviceManager.pickAnySuitableDevice(instance, surface);
		const auto physicalDevice = physicalDeviceManager.getPhysicalDevice();
		assert(physicalDevice);

		const auto [graphicsFamily, presentFamily] = findQueueFamilies(physicalDevice, surface);
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

		const auto requiredExtensions = getRequiredDeviceExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		// Distinction between instance and device specific validation no longer the case. This was added for back compatibility.
		const std::vector<const char*> validationLayers = DebugManager::getValidationLayers();
		if (DebugManager::validationLayersAreEnabled()) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();
		}
		else 
		{
			createInfo.enabledLayerCount = 0;
		}

		if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanLogicalDeviceManager: failed to create logical device.");
		}

	}

	void DeviceManager::clean()
	{
		vkDestroyDevice(logicalDevice, nullptr);
	}

	void DeviceManager::deviceWaitIdle() const
	{
		vkDeviceWaitIdle(logicalDevice);
	}
}
