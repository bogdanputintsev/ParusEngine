#include "Application.h"

#include "engine/Event.h"
#include "engine/input/Input.h"
#include "services/platform/Platform.h"
#include "services/graphics/imgui/ImGuiLibrary.h"
#include "services/graphics/GraphicsLibrary.h"
#include "services/renderer/vulkan/VulkanRenderer.h"
#include "services/Services.h"
#include "services/config/Configs.h"
#include "services/threading/ThreadPool.h"
#include "services/world/World.h"

namespace parus
{
	void Application::init()
	{
		registerServices();
		registerEvents();
		
		Services::get<Configs>()->loadAll();
		
		Services::get<Platform>()->init();
		Services::get<ThreadPool>()->init();
		Services::get<Renderer>()->init();
		Services::get<GraphicsLibrary>()->init();

		isRunning = true;
	}

	void Application::registerServices()
	{
		Services::registerService<Configs>(std::make_shared<Configs>());
		Services::registerService<Platform>(std::make_shared<Platform>());
		Services::registerService<GraphicsLibrary>(std::make_shared<imgui::ImGuiLibrary>());
		Services::registerService<Renderer>(std::make_shared<vulkan::VulkanRenderer>());
		Services::registerService<EventSystem>(std::make_shared<EventSystem>());
		Services::registerService<Input>(std::make_shared<Input>());
		Services::registerService<World>(std::make_shared<World>());
		Services::registerService<ThreadPool>(std::make_shared<ThreadPool>());
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

			Services::get<World>()->tick(deltaTime);
			Services::get<Platform>()->getMessages();

			const bool isMinimized = Services::get<Configs>()->getAsBool("Window", "isMinimized").value_or(false);
			if (!isMinimized)
			{
				Services::get<GraphicsLibrary>()->drawFrame();
				Services::get<Renderer>()->drawFrame();
			}
		}

		Services::get<Renderer>()->deviceWaitIdle();
	}

	void Application::clean()
	{
		isRunning = false;
		
		Services::get<GraphicsLibrary>()->clean();
		Services::get<Renderer>()->clean();
		Services::get<Platform>()->clean();
	}

}
