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

		deviceManager.init(instance, surface); 
		swapChainManager.init(deviceManager, surface, window);
		imageViewManager.init(swapChainManager.getSwapChainImageDetails(), deviceManager.getLogicalDevice());

		clean();
	}

	void Application::clean() const
	{
		imageViewManager.clean(deviceManager.getLogicalDevice());
		swapChainManager.clean(deviceManager.getLogicalDevice());
		deviceManager.clean();
		debugManager.clean(instance);
		surfaceManager.clean(instance);
		vulkanInstanceManager.clean();
		glfwInitializer.clean();
	}

}
