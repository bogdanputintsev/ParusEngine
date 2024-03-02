#include "Application.h"

#include <cassert>

namespace tessera
{
	
	void Application::init()
	{
		glfwInitializer.init();
		vulkanInstanceManager.init();

		instance = vulkanInstanceManager.getInstance();
		assert(instance);

		debugManager.init(instance);
		logicalDeviceManager.init(instance);
		clean();
	}

	void Application::clean() const
	{
		logicalDeviceManager.clean();
		debugManager.clean(instance);
		vulkanInstanceManager.clean();
		glfwInitializer.clean();
	}

}
