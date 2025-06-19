#pragma once
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class VulkanSurfaceManager final
	{
	public:
		static void init();
	private:
		VkSurfaceKHR surface;
	};
	
}

