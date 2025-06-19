#pragma once
#include <vulkan/vulkan_core.h>

#include "VulkanDebugManager.h"

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

		VkInstance instance = nullptr;
		VulkanDebugManager debugManager;
	};
}


