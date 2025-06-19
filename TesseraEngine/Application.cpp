#include "Application.h"

#include <cassert>

namespace tessera
{
	
	void Application::init()
	{
		glfwInitializer.init();
		window = glfwInitializer.getWindow();
		assert(window);

		vulkanInstanceManager.init();
		instance = vulkanInstanceManager.getInstance();
		assert(instance);

		debugManager.init(instance);

		surfaceManager.init(instance, window);
		surface = surfaceManager.getSurface();
		assert(surface);

		logicalDeviceManager.init(instance, surface);
		clean();
	}

	void Application::clean() const
	{
		logicalDeviceManager.clean();
		debugManager.clean(instance);
		surfaceManager.clean(instance);
		vulkanInstanceManager.clean();
		glfwInitializer.clean();
	}

}
