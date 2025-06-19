#pragma once
#include <memory>

#include "VulkanPhysicalDeviceManager.h"

namespace tessera::vulkan
{

	class VulkanLogicalDeviceManager final
	{
	public:
		void init(const std::shared_ptr<const VkInstance>& instance, const std::shared_ptr<VkSurfaceKHR_T* const>& surface);
		void clean() const;

	private:
		VkDevice device = VK_NULL_HANDLE;
		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;
	};
	
}

