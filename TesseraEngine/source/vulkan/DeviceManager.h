#pragma once

#include "PhysicalDeviceManager.h"
#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	class DeviceManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] VkDevice getLogicalDevice() const { return logicalDevice; }
		[[nodiscard]] VkPhysicalDevice getPhysicalDevice() const { return physicalDeviceManager.getPhysicalDevice(); }
	private:
		VkDevice logicalDevice = VK_NULL_HANDLE;
		PhysicalDeviceManager physicalDeviceManager;
	};
	
}

