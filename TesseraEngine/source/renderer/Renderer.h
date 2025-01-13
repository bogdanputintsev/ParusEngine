#pragma once

namespace tessera
{

	class Renderer
	{
	public:
		virtual ~Renderer() = default;

		virtual void init() = 0;
		virtual void clean() = 0;
		virtual void drawFrame() = 0;
		virtual void deviceWaitIdle() = 0;

	};
}
