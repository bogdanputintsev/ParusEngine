#include "VulkanExtensionManager.h"

#include <stdexcept>
#include <unordered_set>
#include <vector>
#include <GLFW/glfw3.h>

#include "VulkanDebugManager.h"

namespace tessera::vulkan
{
	std::vector<const char*> VulkanExtensionManager::getRequiredExtensions()
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		if (VulkanDebugManager::validationLayersAreEnabled()) {
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void VulkanExtensionManager::checkIfAllGlsfRequiredExtensionsAreSupported()
	{
		const std::vector<const char*> requiredExtensions = getRequiredExtensions();
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

}

