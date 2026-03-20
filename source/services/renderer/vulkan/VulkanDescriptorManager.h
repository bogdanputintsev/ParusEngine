#pragma once

#include <unordered_map>

#include "VulkanDescriptor.h"

namespace parus::vulkan
{
	struct VulkanStorage;

	class VulkanDescriptorManager
	{
	public:
		enum class DescriptorType { GLOBAL, INSTANCE, MATERIAL, LIGHTS };

		void define(DescriptorType type, VulkanDescriptor descriptor);

		void setup(VulkanStorage& storage);
		void cleanup(const VulkanStorage& storage);
		void rebuildAll(VulkanStorage& storage);

		[[nodiscard]] VkDescriptorSetLayout getLayout(DescriptorType type) const;

	private:
		std::unordered_map<DescriptorType, VulkanDescriptor> descriptors;
	};

}