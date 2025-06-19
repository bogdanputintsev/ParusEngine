#pragma once
#include <memory>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class VulkanPhysicalDeviceManager final
	{
	public:
		void pickAnySuitableDevice(const std::shared_ptr<const VkInstance>& instance, const std::shared_ptr<const VkSurfaceKHR>& surface);

		[[nodiscard]] std::shared_ptr<const VkPhysicalDevice> getPhysicalDevice() const { return physicalDevice; }

	private:
		static bool isDeviceSuitable(const VkPhysicalDevice& device, const std::shared_ptr<const VkSurfaceKHR>& surface);

		std::shared_ptr<VkPhysicalDevice> physicalDevice;
	};
	
}
