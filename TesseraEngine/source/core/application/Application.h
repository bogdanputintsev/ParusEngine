#pragma once
#include "ApplicationInfo.h"


namespace tessera
{
	
	class Application final
	{
	public:
		void init();
		void registerEvents();
		void loop();
		void clean();

		[[nodiscard]] ApplicationInfo getApplicationInfo() const { return applicationInfo; }
	private:
		bool isRunning = false;

		ApplicationInfo applicationInfo{};
	};

}

