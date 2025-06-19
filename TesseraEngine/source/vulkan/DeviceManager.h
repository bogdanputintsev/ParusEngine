#pragma once
#include <memory>

#include "PhysicalDeviceManager.h"
#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	class DeviceManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] std::shared_ptr<const VkDevice> getLogicalDevice() const { return logicalDevice; }
		[[nodiscard]] std::shared_ptr<const VkPhysicalDevice> getPhysicalDevice() const { return physicalDeviceManager.getPhysicalDevice(); }
	private:
		std::shared_ptr<VkDevice> logicalDevice = VK_NULL_HANDLE;
		PhysicalDeviceManager physicalDeviceManager;
	};
	
}

