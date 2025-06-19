#include "VulkanInstanceManager.h"

#include <stdexcept>
#include <GLFW/glfw3.h>

#include "VulkanDebugManager.h"
#include "VulkanExtensionManager.h"

namespace tessera::vulkan
{

	void VulkanInstanceManager::init()
	{
		createInstance();
	}

	void VulkanInstanceManager::createInstance()
	{
		VulkanDebugManager::checkValidationLayerSupport();

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

		VulkanExtensionManager::checkIfAllGlsfRequiredExtensionsAreSupported();

		const std::vector<const char*> requiredExtensions = VulkanExtensionManager::getRequiredInstanceExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredExtensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

		const std::vector<const char*> validationLayers = VulkanDebugManager::getValidationLayers();
		if (VulkanDebugManager::validationLayersAreEnabled()) 
		{
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			VulkanDebugManager::populate(debugCreateInfo);
			createInfo.pNext = &debugCreateInfo;
		}
		else 
		{
			createInfo.enabledLayerCount = 0;
			createInfo.pNext = nullptr;
		}

		VkInstance instanceObject = VK_NULL_HANDLE;
		if (vkCreateInstance(&createInfo, nullptr, &instanceObject) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanInstanceManager: failed to create instance.");
		}

		instance = std::make_shared<VkInstance>(instanceObject);
	}

	void VulkanInstanceManager::clean() const
	{
		vkDestroyInstance(*instance, nullptr);
	}

}

