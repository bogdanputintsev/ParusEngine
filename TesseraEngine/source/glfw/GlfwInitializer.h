#pragma once

#include <functional>
#include <GLFW/glfw3.h>
#include <memory>
#include <string>

#include "utils/interfaces/Initializable.h"

#define GLFW_INCLUDE_VULKAN

namespace tessera::glfw
{
	class GlfwInitializer final : public Initializable
	{
	public:
		void init() override;
		void mainLoop(const std::function<void()>& tickCallback) const;
		void clean() override;

		[[nodiscard]] std::shared_ptr<GLFWwindow> getWindow() const { return window; }
		void handleMinimization() const;
	private:
		std::shared_ptr<GLFWwindow> window;

		static constexpr int WINDOW_WIDTH = 800;
		static constexpr int WINDOW_HEIGHT = 600;
		static constexpr std::string WINDOW_TITLE = "Tessera Engine";
	};

	void framebufferResizeCallback(GLFWwindow* window, const int width, const int height);

}

