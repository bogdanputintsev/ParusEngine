#pragma once

#include "glfw/GlfwInitializer.h"
#include "vulkan/VulkanDebugManager.h"
#include "vulkan/VulkanFramebufferManager.h"
#include "vulkan/VulkanGraphicsPipelineManager.h"
#include "vulkan/VulkanImageViewManager.h"
#include "vulkan/VulkanInstanceManager.h"
#include "vulkan/VulkanSurfaceManager.h"

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
		vulkan::VulkanGraphicsPipelineManager graphicsPipelineManager;
		vulkan::VulkanFramebufferManager framebufferManager;
	};

}

