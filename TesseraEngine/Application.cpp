#include "Application.h"

namespace tessera
{

	void Application::run()
	{
		glfwInitializer.init();
		vulkanInitializer.init();
		clean();
	}

	void Application::clean() const
	{
		vulkanInitializer.clean();
		glfwInitializer.clean();
	}

}
