#pragma once


namespace parus
{
	
	class Application final
	{
	public:
		void init(int argc, const char* argv[]);
		void loop();
		void clean();
	private:
		static void registerServices();
		void registerEvents();
		static void registerConsoleCommands();
		static void processArgs(int argc, const char* argv[]);
		
		bool isRunning = false;
		bool isMinimized = false;
	};

}

