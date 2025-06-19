#include "VulkanDebugManager.h"

#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{
	void VulkanDebugManager::init(const std::shared_ptr<VkInstance>& instance)
	{
        if (!validationLayersAreEnabled())
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populate(createInfo);

        if (createDebugUtilsMessengerExt(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) 
        {
            throw std::runtime_error("VulkanDebugManager: failed to set up debug messenger!");
        }
	}

    void VulkanDebugManager::populate(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

	void VulkanDebugManager::clean(const std::shared_ptr<VkInstance>& instance) const
	{
        if(validationLayersAreEnabled())
        {
            destroyDebugUtilsMessengerExt(*instance, debugMessenger, nullptr);
        }
	}

	VkBool32 VulkanDebugManager::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	                                           VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                           void* pUserData)
    {
        TesseraLog::send(getLogType(messageSeverity), "Vulkan", pCallbackData->pMessage);
        return VK_FALSE;
    }

	LogType VulkanDebugManager::getLogType(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
	{
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
            return LogType::DEBUG;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: 
            return LogType::INFO;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: 
            return LogType::WARNING;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: 
            return LogType::ERROR;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: 
            return LogType::INFO;
		}

		return LogType::INFO;
	}

	VkResult VulkanDebugManager::createDebugUtilsMessengerExt(const std::shared_ptr<VkInstance>& instance,
	                                                          const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	                                                          const VkAllocationCallbacks* pAllocator,
	                                                          VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
	    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr) 
        {
            return func(*instance, pCreateInfo, pAllocator, pDebugMessenger);
        }

		return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void VulkanDebugManager::destroyDebugUtilsMessengerExt(VkInstance instance,
	    VkDebugUtilsMessengerEXT debugMessenger, 
        const VkAllocationCallbacks* pAllocator)
    {
	    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func != nullptr) 
        {
            func(instance, debugMessenger, pAllocator);
        }
    }

    void VulkanDebugManager::checkValidationLayerSupport()
    {
        if (!validationLayersAreEnabled())
        {
            return;
        }

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        bool result = true;
        
        for (const char* layerName : getValidationLayers()) 
        {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers)
            {
                if (strcmp(layerName, layerProperties.layerName) == 0) 
                {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) 
            {
                result = false;
                break;
            }
        }

        if (!result)
        {
            throw std::runtime_error("VulkanInitializer: validation layers requested, but not available.");
        }
	}

	bool VulkanDebugManager::validationLayersAreEnabled()
	{
#ifdef NDEBUG
        return false;
#else
        return true;
#endif
	}

	std::vector<const char*> VulkanDebugManager::getValidationLayers()
	{
        return { "VK_LAYER_KHRONOS_validation" };
	}
}
