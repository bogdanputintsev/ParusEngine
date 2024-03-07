#pragma once

#include "glfw/GlfwInitializer.h"
#include "vulkan/CommandPoolManager.h"
#include "vulkan/DebugManager.h"
#include "vulkan/FramebufferManager.h"
#include "vulkan/GraphicsPipelineManager.h"
#include "vulkan/ImageViewManager.h"
#include "vulkan/InstanceManager.h"
#include "vulkan/SurfaceManager.h"

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
		vulkan::InstanceManager instanceManager;
		vulkan::DebugManager debugManager;
		vulkan::SurfaceManager surfaceManager;
		vulkan::DeviceManager deviceManager;
		vulkan::SwapChainManager swapChainManager;
		vulkan::ImageViewManager imageViewManager;
		vulkan::GraphicsPipelineManager graphicsPipelineManager;
		vulkan::FramebufferManager framebufferManager;
		vulkan::CommandPoolManager commandPoolManager;
	};

}

