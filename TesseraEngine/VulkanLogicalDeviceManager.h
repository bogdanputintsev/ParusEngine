#pragma once
#include "VulkanPhysicalDeviceManager.h"

namespace tessera::vulkan
{

	class VulkanLogicalDeviceManager final
	{
	public:
		void init(const std::shared_ptr<VkInstance>& instance);
		void clean() const;

	private:
		VkDevice device = VK_NULL_HANDLE;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
	};
	
}

