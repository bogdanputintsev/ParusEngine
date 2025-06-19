#include "SurfaceManager.h"

#include <stdexcept>

#include "InstanceManager.h"
#include "glfw/GlfwInitializer.h"
#include "utils/interfaces/ServiceLocator.h"


namespace tessera::vulkan
{

	void SurfaceManager::init()
	{
		const std::shared_ptr<VkInstance>& instance = ServiceLocator::getService<InstanceManager>()->getInstance();
		const std::shared_ptr<GLFWwindow>& window = ServiceLocator::getService<glfw::GlfwInitializer>()->getWindow();

		VkSurfaceKHR surfaceInstance;
		if (glfwCreateWindowSurface(*instance, window.get(), nullptr, &surfaceInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanSurfaceManager: failed to create window surface.");
		}

		surface = std::make_shared<VkSurfaceKHR>(surfaceInstance);
	}

	void SurfaceManager::clean()
	{
		const std::shared_ptr<VkInstance>& instance = ServiceLocator::getService<InstanceManager>()->getInstance();

		vkDestroySurfaceKHR(*instance, *surface, nullptr);
	}

}
