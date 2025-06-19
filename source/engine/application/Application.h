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
		
		bool isRunning = false;
	};

}

