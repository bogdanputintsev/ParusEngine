#include "Application.h"

#include "Core.h"
#include "core/Input.h"
#include "graphics/imgui/ImGuiLibrary.h"

namespace tessera
{
	void Application::init()
	{
		CORE->platform->init();
		CORE->renderer->init();
		CORE->graphicsLibrary->init();

		isRunning = true;

		registerEvents();
	}

	void Application::registerEvents()
	{
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

	void Application::loop() const
	{
		while (isRunning)
		{
			CORE->platform->getMessages();

			if (!CORE->platform->getWindowInfo().isMinimized)
			{
				CORE->graphicsLibrary->drawFrame();
				CORE->renderer->drawFrame();
			}
		}

		CORE->renderer->deviceWaitIdle();
	}

	void Application::clean()
	{
		isRunning = false;
		CORE->graphicsLibrary->clean();
		CORE->renderer->clean();
		CORE->platform->clean();
	}

}
