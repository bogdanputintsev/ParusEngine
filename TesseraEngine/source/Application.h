#pragma once

namespace tessera
{
	class Application final
	{
	public:
		void init();
		void loop();
		void clean();

	private:
		bool isRunning = false;
	};

}

