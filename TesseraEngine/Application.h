#pragma once

#include "GlfwInitializer.h"
#include "VulkanInitializer.h"

namespace tessera
{
	class Application final
	{
	public:
		Application() = default;

		void run();

	private:
		void clean() const;

		vulkan::VulkanInitializer vulkanInitializer;
		glfw::GlfwInitializer glfwInitializer;
	};

}

