#include "GlfwInitializer.h"

namespace tessera::glfw
{

	void GlfwInitializer::init()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Tessera Engine", nullptr, nullptr);
	}

	void GlfwInitializer::mainLoop() const
	{
		while (!glfwWindowShouldClose(window))
		{
			glfwPollEvents();
		}
	}

	void GlfwInitializer::clean() const
	{
		glfwDestroyWindow(window);
		glfwTerminate();
	}

}
