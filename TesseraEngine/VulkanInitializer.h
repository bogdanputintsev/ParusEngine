#pragma once
#include <vulkan/vulkan_core.h>

#include "VulkanDebugManager.h"
#include "VulkanPhysicalDeviceManager.h"

namespace tessera::vulkan
{

	class VulkanInitializer final
	{
	public:
		VulkanInitializer() = default;
		void init();
		void clean() const;

	private:
		void createInstance();

		VkInstance instance = VK_NULL_HANDLE;
		VulkanDebugManager debugManager;
		VulkanPhysicalDeviceManager physicalDeviceManager;
	};

}


