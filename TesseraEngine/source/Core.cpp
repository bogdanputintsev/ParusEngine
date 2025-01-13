#include "Core.h"

namespace tessera
{

	std::shared_ptr<Core> Core::getInstance()
	{
		static auto instance = std::make_shared<Core>();
		return instance;
	}

	Core::Core()
	{
		context->graphicsLibrary = std::make_shared<glfw::GlfwLibrary>();
		context->renderer = std::make_shared<vulkan::VulkanRenderer>();
	}
}
