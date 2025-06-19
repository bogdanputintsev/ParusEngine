#include "VulkanInitializer.h"

#include <iostream>
#include <stdexcept>
#include <GLFW/glfw3.h>

namespace tessera::vulkan
{

	void VulkanInitializer::init()
	{
		createInstance();
	}

	void VulkanInitializer::createInstance()
	{
		validationLayersManager.checkValidationLayerSupport();

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Sandbox";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Tessera Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		const std::vector<const char*> requiredExtensions = VulkanExtensionManager::getRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		validationLayersManager.includeValidationLayerNamesIfNeeded(createInfo);

		VulkanExtensionManager::checkIfAllGlsfRequiredExtensionsAreSupported();

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanInitializer: failed to create instance.");
		}
	}

	void VulkanInitializer::clean() const
	{
		vkDestroyInstance(instance, nullptr);
	}

}

