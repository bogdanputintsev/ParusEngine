#pragma once


#include <vulkan/vulkan_core.h>
#include <mutex>

namespace parus::vulkan
{
    
    struct VulkanStorage final
    {
        VkInstance instance = VK_NULL_HANDLE;
        VkDebugUtilsMessengerEXT debugMessenger = VK_NULL_HANDLE;
		PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
		VkSurfaceKHR surface = VK_NULL_HANDLE;
		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
		VkDevice logicalDevice = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
        std::mutex graphicsQueueMutex;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
        
    };

}
