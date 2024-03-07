#include "ExtensionManager.h"

#include <set>
#include <stdexcept>
#include <unordered_set>
#include <vector>
#include <GLFW/glfw3.h>

#include "DebugManager.h"

namespace tessera::vulkan
{
	std::vector<const char*> ExtensionManager::getRequiredInstanceExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (DebugManager::validationLayersAreEnabled()) 
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void ExtensionManager::checkIfAllGlsfRequiredExtensionsAreSupported()
	{
		const std::vector<const char*> requiredExtensions = getRequiredInstanceExtensions();
		const std::unordered_set<std::string> requiredExtensionsSet{ requiredExtensions.begin(), requiredExtensions.end() };

		uint32_t matches = 0;

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		for (const auto& [extensionName, specVersion] : extensions)
		{
			// TODO: Custom setting for printing extensions.
			//std::cout << extensionName << " " << requiredExtensionsSet.contains(extensionName) << std::endl;

			if (requiredExtensionsSet.contains(std::string(extensionName)))
			{
				++matches;
			}
		}

		if (matches != requiredExtensions.size())
		{
			throw std::runtime_error("VulkanInitializer: GLFW window extension is not supported.");
		}
	}

	std::vector<const char*> ExtensionManager::getRequiredDeviceExtensions()
	{
		return { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	}

	// TODO: Avoid code redundancy.
	bool ExtensionManager::isDeviceExtensionSupported(const VkPhysicalDevice& device)
	{
		const std::vector<const char*> requiredExtensions = getRequiredDeviceExtensions();

		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		// TODO: Optimize with O(1) complexity.
		std::set<std::string> requiredExtensionsSet(requiredExtensions.begin(), requiredExtensions.end());

		for (const auto& extension : availableExtensions) 
		{
			requiredExtensionsSet.erase(extension.extensionName);
		}

		return requiredExtensionsSet.empty();
	}
}

