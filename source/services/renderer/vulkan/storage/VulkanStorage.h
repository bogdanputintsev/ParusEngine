#pragma once
#include <vulkan/vulkan_core.h>

namespace parus::vulkan
{
    
    struct VulkanStorage final
    {
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;

        
        VkDevice device = VK_NULL_HANDLE;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;

    };

}
