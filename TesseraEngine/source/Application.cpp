#include "Application.h"

#include "Core.h"
#include "core/Input.h"

namespace tessera
{
	void Application::init()
	{
		CORE->platform->init("Tessera", 250, 250, 800, 600);
		//CORE->graphicsLibrary->init();
		CORE->renderer->init();

		isRunning = true;

		REGISTER_EVENT(EventType::EVENT_KEY_RELEASED, [](const KeyButton keyReleased)
		{
			if (keyReleased == KeyButton::KEY_ESCAPE)
			{
				FIRE_EVENT(EventType::EVENT_APPLICATION_QUIT, 0);
			}
		});
		
		REGISTER_EVENT(EventType::EVENT_APPLICATION_QUIT, [&]([[maybe_unused]]const int exitCode)
		{
			isRunning = false;
		});
	}

	void Application::loop()
	{
		while (isRunning)
		{
			CORE->platform->getMessages();
			CORE->renderer->drawFrame();
		}
		// CORE->graphicsLibrary->loop([&]
		// 	{
		// 		CORE->renderer->drawFrame();
		// 	});

		CORE->renderer->deviceWaitIdle();
	}

	void Application::clean()
	{
		CORE->renderer->clean();
		//CORE->graphicsLibrary->clean();
		CORE->platform->clean();
	}

}
