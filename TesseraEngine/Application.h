#pragma once

#include "GlfwInitializer.h"
#include "VulkanDebugManager.h"
#include "VulkanImageViewManager.h"
#include "VulkanInstanceManager.h"
#include "VulkanSurfaceManager.h"
#include "VulkanSwapChainManager.h"

namespace tessera
{
	class Application final
	{
	public:
		Application() = default;
		void init();

	private:
		void clean() const;

		std::shared_ptr<GLFWwindow> window;
		std::shared_ptr<const VkInstance> instance;
		std::shared_ptr<const VkSurfaceKHR> surface;

		glfw::GlfwInitializer glfwInitializer;
		vulkan::VulkanInstanceManager vulkanInstanceManager;
		vulkan::VulkanDebugManager debugManager;
		vulkan::VulkanSurfaceManager surfaceManager;
		vulkan::VulkanDeviceManager deviceManager;
		vulkan::VulkanSwapChainManager swapChainManager;
		vulkan::VulkanImageViewManager imageViewManager;
	};

}

