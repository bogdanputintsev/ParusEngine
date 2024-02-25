#pragma once

#define GLFW_INCLUDE_VULKAN

#include <GLFW/glfw3.h>

namespace tessera::glfw
{
	class GlfwInitializer final
	{
	public:
		void init();
		void mainLoop() const;
		void clean() const;

	private:
		GLFWwindow* window = nullptr;

		static constexpr int WINDOW_WIDTH = 800;
		static constexpr int WINDOW_HEIGHT = 600;
	};
}

