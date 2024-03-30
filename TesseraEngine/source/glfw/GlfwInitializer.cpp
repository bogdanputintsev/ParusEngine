#include "GlfwInitializer.h"

#include <functional>
#include <stdexcept>

#include "utils/interfaces/ServiceLocator.h"

namespace tessera::glfw
{

	void GlfwInitializer::init()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		GLFWwindow* windowObject = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE.c_str(), nullptr, nullptr);
		if (!windowObject)
		{
			throw std::runtime_error("GlfwInitializer: failed to initialize window.");
		}

		window = std::shared_ptr<GLFWwindow>(windowObject, [](GLFWwindow*)
		{
			// TODO: rewrite init/clean methods with constructor and destructor.
			// Empty destructor.
		});
	}

	void GlfwInitializer::mainLoop(const std::function<void()>& tickCallback) const
	{
		while (!glfwWindowShouldClose(window.get()))
		{
			glfwPollEvents();
			tickCallback();
		}
	}

	void GlfwInitializer::clean()
	{
		glfwDestroyWindow(window.get());
		glfwTerminate();
	}

	
}
