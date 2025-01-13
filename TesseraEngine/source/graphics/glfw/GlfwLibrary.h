#pragma once
#include <memory>
#include <string>
#include <GLFW/glfw3.h>

#include "graphics/GraphicsLibrary.h"

namespace tessera::glfw
{

	class GlfwLibrary final : public GraphicsLibrary
	{
	public:
		virtual ~GlfwLibrary() override = default;

		void init() override;
		void loop(const std::function<void()>& tickCallback) override;
		void handleMinimization() override;
		[[nodiscard]] std::vector<const char*> getRequiredExtensions() const override;
		[[nodiscard]] std::shared_ptr<GLFWwindow> getWindow() const { return window; }

		void clean() override;

	private:
		std::shared_ptr<GLFWwindow> window;

	};

	void framebufferResizeCallback(GLFWwindow* window, const int width, const int height);

}

