#pragma once

#include "GlfwInitializer.h"
#include "VulkanDebugManager.h"
#include "VulkanInstanceManager.h"

namespace tessera
{
	class Application final
	{
	public:
		Application() = default;
		void init();

	private:
		void clean() const;

		std::shared_ptr<VkInstance> instance;

		glfw::GlfwInitializer glfwInitializer;
		vulkan::VulkanInstanceManager vulkanInstanceManager;
		vulkan::VulkanDebugManager debugManager;
		vulkan::VulkanLogicalDeviceManager logicalDeviceManager;
	};

}

