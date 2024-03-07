#pragma once
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{
	
	class CommandPoolManager final
	{
	public:
		void init();
		void clean() const;
	private:
		VkCommandPool commandPool = VK_NULL_HANDLE;
	};

}

