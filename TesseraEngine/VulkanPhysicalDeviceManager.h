#pragma once
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class VulkanPhysicalDeviceManager final
	{
	public:
		void pickAnySuitableDevice(const VkInstance& instance);

	private:
		static bool isDeviceSuitable(const VkPhysicalDevice& device);

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	};
	
}
