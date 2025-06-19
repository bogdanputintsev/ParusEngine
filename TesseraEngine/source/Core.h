#pragma once
#include <memory>

#include "Event.h"
#include "utils/TesseraLog.h"
#include "utils/Utils.h"
#include "graphics/glfw/GlfwLibrary.h"
#include "renderer/vulkan/VulkanRenderer.h"

namespace tessera
{
	class Core final
	{
	public:
		static std::shared_ptr<Core> getInstance();

		Core(Core&) = delete;
		Core(Core&&) = delete;
		Core& operator=(Core&) = delete;
		Core& operator=(Core&&) = delete;
		Core();
		~Core() = default;

	private:

		struct CoreContext final
		{
			std::shared_ptr<glfw::GlfwLibrary> graphicsLibrary;
			std::shared_ptr<vulkan::VulkanRenderer> renderer;
			std::shared_ptr<EventSystem> eventSystem = std::make_shared<EventSystem>();
		};

	public:
		std::unique_ptr<CoreContext> context = std::make_unique<CoreContext>();
	};

#define CORE Core::getInstance()->context
}


