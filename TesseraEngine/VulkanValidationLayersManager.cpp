#include "VulkanValidationLayersManager.h"

#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	void VulkanValidationLayersManager::checkValidationLayerSupport() const
	{
        if (!isEnabled())
        {
            return;
        }

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        bool result = true;

        for (const char* layerName : validationLayers) 
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

	void VulkanValidationLayersManager::includeValidationLayerNamesIfNeeded(VkInstanceCreateInfo& createInfo) const
	{
        if (validationLayersEnabled) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        }
        else {
            createInfo.enabledLayerCount = 0;
        }
	}

	bool VulkanValidationLayersManager::isEnabled()
	{
#ifdef NDEBUG
        return false;
#else
        return true;
#endif
	}
}
