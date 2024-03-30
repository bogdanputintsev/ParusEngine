#pragma once
#define GLFW_INCLUDE_VULKAN

#include <vulkan/vulkan_core.h>

#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	class SurfaceManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;
	
		[[nodiscard]] VkSurfaceKHR getSurface() const { return surface; }
	private:
		VkSurfaceKHR surface = VK_NULL_HANDLE;
	};
	
}

