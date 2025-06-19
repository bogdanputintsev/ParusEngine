#pragma once

namespace tessera
{
	class Application final
	{
	public:
		void init();
		void registerEvents();
		void loop() const;
		void clean();

	private:
		bool isRunning = false;
	};

}

