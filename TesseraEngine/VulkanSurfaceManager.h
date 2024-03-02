#pragma once
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <memory>
#include <memory>
#include <vulkan/vulkan_core.h>

namespace tessera::vulkan
{

	class VulkanSurfaceManager final
	{
	public:
		void init(const std::shared_ptr<const VkInstance>& instance, const std::shared_ptr<GLFWwindow>& window);
		void clean(const std::shared_ptr<const VkInstance>& instance) const;

		[[nodiscard]] std::shared_ptr<const VkSurfaceKHR> getSurface() const { return surface; }
	private:
		std::shared_ptr<VkSurfaceKHR> surface;
	};
	
}

