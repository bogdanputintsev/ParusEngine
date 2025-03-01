#pragma once
#include <memory>

#include "core/Defines.h"
#include "Event.h"
#include "core/platform/Platform.h"
#include "graphics/imgui/ImGuiLibrary.h"
#include "utils/TesseraLog.h"
#include "utils/Utils.h"
#include "renderer/vulkan/VulkanRenderer.h"
#include "world/World.h"

namespace tessera
{
	class Input;

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
			std::shared_ptr<Platform> platform;
			std::shared_ptr<imgui::ImGuiLibrary> graphicsLibrary;
			std::shared_ptr<vulkan::VulkanRenderer> renderer;
			std::shared_ptr<EventSystem> eventSystem;
			std::shared_ptr<Input> inputSystem;
			World world;
		};

	public:
		std::unique_ptr<CoreContext> context = std::make_unique<CoreContext>();
	};

#define CORE Core::getInstance()->context
}


