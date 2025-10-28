#pragma once


#include <vulkan/vulkan_core.h>
#include <mutex>

#include "services/renderer/vulkan/utils/VulkanUtils.h"

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
		utils::SwapChainImageDetails swapChainDetails{};
		VkSwapchainKHR swapChain = VK_NULL_HANDLE;
		VkRenderPass renderPass = VK_NULL_HANDLE;

    	// Descriptors
    	VkDescriptorSetLayout globalDescriptorSetLayout = VK_NULL_HANDLE;
    	VkDescriptorSetLayout instanceDescriptorSetLayout = VK_NULL_HANDLE;
    	VkDescriptorSetLayout materialDescriptorSetLayout = VK_NULL_HANDLE;
    	VkDescriptorSetLayout lightsDescriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorPool descriptorPool;
    	
    };

}
