#pragma once

#include <memory>

namespace parus
{
	class Configs;
	class Platform;
	class GraphicsLibrary;
	class Renderer;
	class EventSystem;
	class Input;
	class World;
	class ThreadPool;
	class Console;
	class Serialization;

	/**
	 * Main application class that manages the engine lifecycle: initialization, main loop, and shutdown.
	 * Registers core services (graphics, rendering, input, world, etc.), handles platform events,
	 * and coordinates frame rendering through the main loop.
	 */
	class Application final
	{
	public:
		/** Initialize the application: register services, load config, create window, initialize Vulkan and ImGui. */
		void init(int argc, const char* argv[]);

		/** Run the main loop: process delta time, tick world, handle platform messages, render frame. */
		void loop();

		/** Perform ordered shutdown of all services and subsystems. */
		void clean();

	private:
		/** Register all core services (Configs, Platform, Graphics, Renderer, Events, Input, World, etc.) with the service locator. */
		void registerServices();

		/** Register event observers for application lifecycle and system events. */
		void registerEvents();

		/** Register console commands for debugging and development (e.g., scene save/load, world inspection). */
		void registerConsoleCommands();

		/** Parse command-line arguments and apply configuration overrides. */
		void processArgs(int argc, const char* argv[]);

		/** Whether the application should continue running the main loop. */
		bool isRunning = false;

		/** Whether the window is currently minimized; used to skip frame rendering. */
		bool isMinimized = false;

		/** Loads and provides access to INI config values. */
		std::shared_ptr<Configs> configs;

		/** OS-level window and message pump abstraction. */
		std::shared_ptr<Platform> platform;

		/** ImGui layer; draws UI on top of the rendered frame. */
		std::shared_ptr<GraphicsLibrary> graphicsLibrary;

		/** Vulkan renderer; owns the swap chain and draws each frame. */
		std::shared_ptr<Renderer> renderer;

		/** Type-safe observer-style event dispatcher used by REGISTER_EVENT/FIRE_EVENT. */
		std::shared_ptr<EventSystem> eventSystem;

		/** Reads platform input and exposes it in engine-friendly form. */
		std::shared_ptr<Input> input;

		/** Scene contents (entities, mesh instances, lights, camera) ticked every frame. */
		std::shared_ptr<World> world;

		/** Worker threads for fire-and-forget async work. */
		std::shared_ptr<ThreadPool> threadPool;

		/** In-engine command console; registers and dispatches debug commands. */
		std::shared_ptr<Console> console;

		/** Saves and loads scenes to/from the engine's binary format. */
		std::shared_ptr<Serialization> serialization;
	};

}

