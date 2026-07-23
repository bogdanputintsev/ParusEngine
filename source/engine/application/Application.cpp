#include "Application.h"

#include "engine/EngineCore.h"
#include "engine/Event.h"
#include "engine/input/Input.h"
#include "services/platform/Platform.h"
#include "services/platform/PlatformWindows.h"
#include "services/graphics/imgui/ImGuiLibrary.h"
#include "services/graphics/GraphicsLibrary.h"
#include "services/renderer/vulkan/VulkanRenderer.h"
#include "services/Services.h"
#include "services/config/Configs.h"
#include "services/console/Console.h"
#include "services/threading/ThreadPool.h"
#include "services/world/World.h"
#include "services/serialization/Serialization.h"

namespace parus
{
	void Application::init(const int argc, const char* argv[])
	{
		registerServices();
		registerEvents();
		registerConsoleCommands();

		configs->loadAll();
		platform->init();
		threadPool->init();
		world->init();
		renderer->init();
		graphicsLibrary->init();

		processArgs(argc, argv);

		isMinimized = configs->getOrDefault<bool>("Window", "isMinimized", false);
		isRunning = true;
	}

	void Application::registerServices()
	{
		configs = std::make_shared<Configs>();
		Services::registerService<Configs>(configs);

#ifdef WITH_WINDOWS_PLATFORM
		platform = std::make_shared<PlatformWindows>();
#else
		FATAL_ERROR("No Platform implementation for this platform.");
#endif
		Services::registerService<Platform>(platform);

		graphicsLibrary = std::make_shared<imgui::ImGuiLibrary>();
		Services::registerService<GraphicsLibrary>(graphicsLibrary);

		renderer = std::make_shared<vulkan::VulkanRenderer>();
		Services::registerService<Renderer>(renderer);

		eventSystem = std::make_shared<EventSystem>();
		Services::registerService<EventSystem>(eventSystem);

		input = std::make_shared<Input>();
		Services::registerService<Input>(input);

		world = std::make_shared<World>();
		Services::registerService<World>(world);

		threadPool = std::make_shared<ThreadPool>();
		Services::registerService<ThreadPool>(threadPool);

		console = std::make_shared<Console>();
		Services::registerService<Console>(console);

		serialization = std::make_shared<Serialization>();
		Services::registerService<Serialization>(serialization);
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
		
		REGISTER_EVENT(EventType::EVENT_WINDOW_MINIMIZED, [&](const bool newIsMinimized)
		{
			isMinimized = newIsMinimized;
		});
	}

	void Application::registerConsoleCommands()
	{
		console->registerConsoleCommand("exit", [](const auto& /*args*/)
		{
			FIRE_EVENT(EventType::EVENT_APPLICATION_QUIT, 0);
			return std::string();
		});

		serialization->registerConsoleCommands();
	}
	
	void Application::processArgs(const int argc, const char* argv[])
	{
		if (argc > 1)
		{
			serialization->importWorld(argv[1]);
		}
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

			world->tick(deltaTime);
			platform->getMessages();

			if (!isMinimized)
			{
				graphicsLibrary->drawFrame();
				renderer->drawFrame();
			}
		}

		renderer->deviceWaitIdle();
	}

	void Application::clean()
	{
		isRunning = false;

		graphicsLibrary->clean();
		renderer->clean();
		platform->clean();
	}

}
