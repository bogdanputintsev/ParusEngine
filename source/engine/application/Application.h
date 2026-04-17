#pragma once


namespace parus
{
	
	class Application final
	{
	public:
		void init();
		void loop();
		void clean();
	private:
		static void registerServices();
		void registerEvents();
		static void registerConsoleCommands();
		
		bool isRunning = false;
		bool isMinimized = false;
	};

}

