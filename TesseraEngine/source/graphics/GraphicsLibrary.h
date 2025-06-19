#pragma once
#include <functional>
#include <string>

namespace tessera
{
	struct WindowSettings
	{
		std::string title = "TesseraEngine";
		int width = 800;
		int height = 600;
	};

	class GraphicsLibrary
	{
	public:
		virtual ~GraphicsLibrary() = default;

		virtual void init() = 0;
		virtual void loop(const std::function<void()>& tickCallback) = 0;
		virtual void handleMinimization() = 0;
		[[nodiscard]] virtual std::vector<const char*> getRequiredExtensions() const = 0;
		virtual void clean() = 0;

	protected:
		[[nodiscard]] WindowSettings getWindowSettings() const { return windowSettings; }

	private:
		WindowSettings windowSettings;
	};

}

