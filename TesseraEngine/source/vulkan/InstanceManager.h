#pragma once
#include <vulkan/vulkan_core.h>

#include "DeviceManager.h"
#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	class InstanceManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] std::shared_ptr<VkInstance> getInstance() const { return instance; }
	private:
		void createInstance();

		std::shared_ptr<VkInstance> instance = VK_NULL_HANDLE;
	};

}


