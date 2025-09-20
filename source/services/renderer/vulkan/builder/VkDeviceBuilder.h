#pragma once


#include <array>

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkDeviceBuilder
    {
    public:
        static void build(VulkanStorage& storage);
  
    private:
        static void pickAnySuitableDevice(VulkanStorage& storage);
		static bool isDeviceSuitable(const VkPhysicalDevice& device, VulkanStorage& storage);
        static bool isDeviceExtensionSupported(const VkPhysicalDevice& device);
        static VkSampleCountFlagBits getMaxUsableSampleCount(const VulkanStorage& storage);
        
        static constexpr std::array<const char*, 1> REQUIRED_DEVICE_EXTENSIONS = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
    };

   
}

