#include "Application.h"

#include "Core.h"

namespace tessera
{
	void Application::init()
	{
		CORE->graphicsLibrary->init();
		CORE->renderer->init();
	}

	void Application::loop()
	{
		CORE->graphicsLibrary->loop([&]
			{
				CORE->renderer->drawFrame();
			});

		CORE->renderer->deviceWaitIdle();
	}

	void Application::clean()
	{
		CORE->renderer->clean();
		CORE->graphicsLibrary->clean();
	}

}
