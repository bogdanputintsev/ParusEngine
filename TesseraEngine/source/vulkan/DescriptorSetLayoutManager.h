#pragma once

#include <vulkan/vulkan_core.h>

#include "utils/interfaces/Initializable.h"

namespace tessera::vulkan
{

	class DescriptorSetLayoutManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] VkDescriptorSetLayout getDescriptorSetLayout() const { return descriptorSetLayout; }
	private:
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
	};
	
}

