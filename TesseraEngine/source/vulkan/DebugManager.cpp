#include "DebugManager.h"

#include <stdexcept>
#include <vulkan/vulkan_core.h>

#include "InstanceManager.h"
#include "utils/interfaces/ServiceLocator.h"

namespace tessera::vulkan
{

	void DebugManager::init()
	{
        if (!validationLayersAreEnabled())
        {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        populate(createInfo);

        if (createDebugUtilsMessengerExt(&createInfo, nullptr, &debugMessenger) != VK_SUCCESS) 
        {
            throw std::runtime_error("VulkanDebugManager: failed to set up debug messenger!");
        }
	}

    void DebugManager::populate(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
    {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

	void DebugManager::clean()
	{
        if(validationLayersAreEnabled())
        {
            destroyDebugUtilsMessengerExt(debugMessenger, nullptr);
        }
	}

	VkBool32 DebugManager::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
												[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType,
	                                           const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	                                           [[maybe_unused]] void* pUserData)
    {
        TesseraLog::send(getLogType(messageSeverity), "Vulkan", pCallbackData->pMessage);
        return VK_FALSE;
    }

	LogType DebugManager::getLogType(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
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

	VkResult DebugManager::createDebugUtilsMessengerExt(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
	                                                          const VkAllocationCallbacks* pAllocator,
	                                                          VkDebugUtilsMessengerEXT* pDebugMessenger)
    {
        const std::shared_ptr<const VkInstance> instance = ServiceLocator::getService<InstanceManager>()->getInstance();
        
	    const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr) 
        {
            return func(*instance, pCreateInfo, pAllocator, pDebugMessenger);
        }

		return VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void DebugManager::destroyDebugUtilsMessengerExt(VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
    {
        const std::shared_ptr<const VkInstance> instance = ServiceLocator::getService<InstanceManager>()->getInstance();

	    const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(*instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func != nullptr)
        {
            func(*instance, debugMessenger, pAllocator);
        }
    }

    void DebugManager::checkValidationLayerSupport()
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

	bool DebugManager::validationLayersAreEnabled()
	{
#ifdef NDEBUG
        return false;
#else
        return true;
#endif
	}

	std::vector<const char*> DebugManager::getValidationLayers()
	{
        return { "VK_LAYER_KHRONOS_validation" };
	}
}
