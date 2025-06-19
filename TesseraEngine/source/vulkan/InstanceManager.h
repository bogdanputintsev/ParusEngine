#pragma once
#include <vulkan/vulkan_core.h>

#include "DeviceManager.h"

namespace tessera::vulkan
{

	class InstanceManager final
	{
	public:
		InstanceManager() = default;
		void init();
		void clean() const;

		[[nodiscard]] std::shared_ptr<VkInstance> getInstance() const { return instance; }
	private:
		void createInstance();

		std::shared_ptr<VkInstance> instance = VK_NULL_HANDLE;
	};

}


