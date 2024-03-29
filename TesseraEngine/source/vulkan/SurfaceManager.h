#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <memory>
#include <vulkan/vulkan_core.h>

#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	class SurfaceManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;
	
		[[nodiscard]] std::shared_ptr<const VkSurfaceKHR> getSurface() const { return surface; }
	private:
		std::shared_ptr<VkSurfaceKHR> surface;
	};
	
}

