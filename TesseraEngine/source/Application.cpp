#include "Application.h"

#include <algorithm>

#include "utils/interfaces/ServiceLocator.h"
#include "vulkan/QueueManager.h"

namespace tessera
{
	
	void Application::init()
	{
		std::ranges::for_each(initializerList.begin(), initializerList.end(), [&](const std::shared_ptr<Initializable>& service)
			{
				service->init();
				registerServiceManager(service.get(), service);
			});
	}

	template<typename T>
	void Application::registerServiceManager(const T* servicePointer, const std::shared_ptr<Initializable>& service)
	{
		ServiceLocator::registerService<T>(servicePointer, service);
	}

	void Application::loop() const
	{
		const auto& glfwInitializer = ServiceLocator::getService<glfw::GlfwInitializer>();
		const auto& queueManager = ServiceLocator::getService<vulkan::QueueManager>();
		const auto& deviceManager = ServiceLocator::getService<vulkan::DeviceManager>();

		glfwInitializer->mainLoop([&] 
			{
				queueManager->drawFrame();
			});

		deviceManager->deviceWaitIdle();
	}

	void Application::clean()
	{
		for (auto it = initializerList.rbegin(); it != initializerList.rend(); ++it)
		{
			(*it)->clean();
		}
	}

}
