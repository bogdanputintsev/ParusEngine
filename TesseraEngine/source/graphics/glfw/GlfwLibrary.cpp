#include "GlfwLibrary.h"

#include <stdexcept>

#include "Core.h"
#include "utils/TesseraLog.h"

namespace tessera::glfw
{

	void GlfwLibrary::init()
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

		const auto [title, width, height] = getWindowSettings();

		GLFWwindow* windowObject = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
		ASSERT(windowObject, "failed to initialize window");

		glfwSetWindowUserPointer(windowObject, this);
		glfwSetFramebufferSizeCallback(windowObject, framebufferResizeCallback);

		window = std::shared_ptr<GLFWwindow>(windowObject, [](GLFWwindow*)
			{
				// Empty destructor.
			});
	}

	void GlfwLibrary::loop(const std::function<void()>& tickCallback)
	{
		while (!glfwWindowShouldClose(window.get()))
		{
			glfwPollEvents();
			tickCallback();
		}
	}

	void GlfwLibrary::handleMinimization()
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

	std::vector<const char*> GlfwLibrary::getRequiredExtensions() const
	{
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector result(glfwExtensions, glfwExtensions + glfwExtensionCount);

		return result;
	}

	void GlfwLibrary::clean()
	{
		glfwDestroyWindow(window.get());
		glfwTerminate();
	}

	void framebufferResizeCallback([[maybe_unused]] GLFWwindow* window, const int width, const int height)
	{
		REGISTER_EVENT(EventType::EVENT_WINDOW_RESIZED, [&](const int newWidth, const int newHeight)
		{
			DEBUG("Window resized. New size: (" + std::to_string(newWidth) + ", " + std::to_string(newHeight) + ").");
		});

		FIRE_EVENT(EventType::EVENT_WINDOW_RESIZED, width, height);
	}
}
