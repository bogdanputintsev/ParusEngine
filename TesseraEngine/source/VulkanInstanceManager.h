#pragma once
#include <vulkan/vulkan_core.h>

#include "VulkanDeviceManager.h"

namespace tessera::vulkan
{

	class VulkanInstanceManager final
	{
	public:
		VulkanInstanceManager() = default;
		void init();
		void clean() const;

		[[nodiscard]] std::shared_ptr<VkInstance> getInstance() const { return instance; }
	private:
		void createInstance();

		std::shared_ptr<VkInstance> instance = VK_NULL_HANDLE;
	};

}


