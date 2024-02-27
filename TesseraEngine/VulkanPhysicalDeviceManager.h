#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class VulkanPhysicalDeviceManager final
	{
	public:
		static VkPhysicalDevice pickAnySuitableDevice(const VkInstance& instance);
		
	private:
		static bool isDeviceSuitable(const VkPhysicalDevice& device);
	};
	
}
