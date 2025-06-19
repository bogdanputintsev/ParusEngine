#pragma once

#include <GLFW/glfw3.h>
#include <memory>
#include <string>

#define GLFW_INCLUDE_VULKAN

namespace tessera::glfw
{
	class GlfwInitializer final
	{
	public:
		void init();
		void mainLoop() const;
		void clean() const;
		[[nodiscard]] std::shared_ptr<GLFWwindow> getWindow() const { return window; }
	private:
		std::shared_ptr<GLFWwindow> window;

		static constexpr int WINDOW_WIDTH = 800;
		static constexpr int WINDOW_HEIGHT = 600;
		static constexpr std::string WINDOW_TITLE = "Tessera Engine";
	};
}

