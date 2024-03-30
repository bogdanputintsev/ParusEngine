#include "GlfwInitializer.h"

#include <functional>
#include <stdexcept>

#include "utils/TesseraLog.h"
#include "utils/interfaces/ServiceLocator.h"
#include "vulkan/QueueManager.h"

namespace tessera::glfw
{

	void GlfwInitializer::init()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		GLFWwindow* windowObject = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Tessera Engine", nullptr, nullptr);
		if (!windowObject)
		{
			throw std::runtime_error("GlfwInitializer: failed to initialize window.");
		}

		window = std::shared_ptr<GLFWwindow>(windowObject, [](GLFWwindow*)
		{
			// TODO: rewrite init/clean methods with constructor and destructor.
			// Empty destructor.
		});

		glfwSetFramebufferSizeCallback(windowObject, framebufferResizeCallback);
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

	void GlfwInitializer::handleMinimization() const
	{
		int width = 0;
		int height = 0;
		glfwGetFramebufferSize(window.get(), &width, &height);

		while (width == 0 || height == 0) 
		{
			glfwGetFramebufferSize(window.get(), &width, &height);
			glfwWaitEvents();
		}
	}

	void framebufferResizeCallback([[maybe_unused]] GLFWwindow* window, [[maybe_unused]] const int width, [[maybe_unused]] const int height)
	{
		ServiceLocator::getService<vulkan::QueueManager>()->onFramebufferResized();
		TesseraLog::send(LogType::INFO, "GlfwInitializer", "User has resized the window. Window dimension: (" + std::to_string(width) + ", " + std::to_string(height) + ").");
	}
}
