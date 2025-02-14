#include "Core.h"

#include "core/Input.h"

namespace tessera
{

	std::shared_ptr<Core> Core::getInstance()
	{
		static auto instance = std::make_shared<Core>();
		return instance;
	}

	Core::Core()
	{
		context->platform = std::make_shared<Platform>();
		//context->graphicsLibrary = std::make_shared<glfw::GlfwLibrary>();
		context->renderer = std::make_shared<vulkan::VulkanRenderer>();
		context->eventSystem = std::make_shared<EventSystem>();
		context->inputSystem = std::make_shared<Input>();
	}
}
