#pragma once
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class PhysicalDeviceManager final
	{
	public:
		void pickAnySuitableDevice(const VkInstance& instance, const VkSurfaceKHR& surface);

		[[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return physicalDevice; }

	private:
		static bool isDeviceSuitable(const VkPhysicalDevice& device, const VkSurfaceKHR& surface);

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
	};
	
}
