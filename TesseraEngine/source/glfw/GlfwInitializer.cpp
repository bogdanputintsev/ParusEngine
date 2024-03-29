#include "GlfwInitializer.h"

#include <stdexcept>

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

		window = std::shared_ptr<GLFWwindow>(windowObject, [](GLFWwindow* ptr)
		{
			// TODO: rewrite init/clean methods with constructor and destructor.
			// Empty destructor.
		});
	}

	void GlfwInitializer::mainLoop() const
	{
		while (!glfwWindowShouldClose(window.get()))
		{
			glfwPollEvents();
		}
	}

	void GlfwInitializer::clean()
	{
		glfwDestroyWindow(window.get());
		glfwTerminate();
	}

}
