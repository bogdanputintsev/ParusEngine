#include "SurfaceManager.h"

#include <stdexcept>

#include "InstanceManager.h"
#include "glfw/GlfwInitializer.h"
#include "utils/interfaces/ServiceLocator.h"

namespace tessera::vulkan
{

	void SurfaceManager::init()
	{
		const auto& instance = ServiceLocator::getService<InstanceManager>()->getInstance();
		const std::shared_ptr<GLFWwindow>& window = ServiceLocator::getService<glfw::GlfwInitializer>()->getWindow();

		if (glfwCreateWindowSurface(instance, window.get(), nullptr, &surface) != VK_SUCCESS)
		{
			throw std::runtime_error("VulkanSurfaceManager: failed to create window surface.");
		}
	}

	void SurfaceManager::clean()
	{
		const auto& instance = ServiceLocator::getService<InstanceManager>()->getInstance();

		vkDestroySurfaceKHR(instance, surface, nullptr);
	}

}
