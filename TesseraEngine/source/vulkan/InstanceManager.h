#pragma once
#include <vulkan/vulkan_core.h>

#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	class InstanceManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] VkInstance getInstance() const { return instance; }
	private:
		void createInstance();

		VkInstance instance = VK_NULL_HANDLE;
	};

}


