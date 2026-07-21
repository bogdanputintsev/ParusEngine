#pragma once


namespace parus
{

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
		static void registerServices();

		/** Register event observers for application lifecycle and system events. */
		void registerEvents();

		/** Register console commands for debugging and development (e.g., scene save/load, world inspection). */
		static void registerConsoleCommands();

		/** Parse command-line arguments and apply configuration overrides. */
		static void processArgs(int argc, const char* argv[]);

		/** Whether the application should continue running the main loop. */
		bool isRunning = false;

		/** Whether the window is currently minimized; used to skip frame rendering. */
		bool isMinimized = false;
	};

}

