#pragma once
#include <memory>

#include "PhysicalDeviceManager.h"

namespace tessera::vulkan
{

	class DeviceManager final
	{
	public:
		void init(const std::shared_ptr<const VkInstance>& instance, const std::shared_ptr<VkSurfaceKHR_T* const>& surface);
		void clean() const;

		[[nodiscard]] std::shared_ptr<const VkDevice> getLogicalDevice() const { return logicalDevice; }
		[[nodiscard]] std::shared_ptr<const VkPhysicalDevice> getPhysicalDevice() const { return physicalDeviceManager.getPhysicalDevice(); }
	private:
		std::shared_ptr<VkDevice> logicalDevice = VK_NULL_HANDLE;

		PhysicalDeviceManager physicalDeviceManager;

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
	};
	
}

