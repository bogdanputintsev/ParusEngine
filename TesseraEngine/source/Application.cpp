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

	void Application::loop()
	{
		auto lastFrameTime = std::chrono::high_resolution_clock::now();

		while (isRunning)
		{
			const auto currentFrameTime = std::chrono::high_resolution_clock::now();
			const std::chrono::duration<float> deltaTimeDuration = currentFrameTime - lastFrameTime;
			const float deltaTime = deltaTimeDuration.count();
			lastFrameTime = currentFrameTime;

			CORE->world.tick(deltaTime);
			
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
