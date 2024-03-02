#include "VulkanSurfaceManager.h"

#include <stdexcept>

namespace tessera::vulkan
{

	void VulkanSurfaceManager::init(const std::shared_ptr<const VkInstance>& instance, const std::shared_ptr<GLFWwindow>& window)
	{
		VkSurfaceKHR surfaceInstance;
		if (glfwCreateWindowSurface(*instance, window.get(), nullptr, &surfaceInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanSurfaceManager: failed to create window surface.");
		}

		surface = std::make_shared<VkSurfaceKHR>(surfaceInstance);
	}

	void VulkanSurfaceManager::clean(const std::shared_ptr<const VkInstance>& instance) const
	{
		vkDestroySurfaceKHR(*instance, *surface, nullptr);
	}

}
