#pragma once

#include <functional>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "builder/VkDescriptorSetLayoutBuilder.h"

namespace parus::vulkan
{
	struct VulkanStorage;

	class VulkanDescriptor final
	{
	public:
		VulkanDescriptor& withLayout(std::string name,
		                             std::vector<VulkanDescriptorSetLayoutBinding> bindings);

		VulkanDescriptor& withPoolSizes(std::vector<VkDescriptorPoolSize> sizes, uint32_t maxSets);

		VulkanDescriptor& withAllocator(std::function<void(VulkanStorage&, VkDescriptorSetLayout)> fn);

	private:
		friend class VulkanDescriptorManager;

		std::string layoutName;
		std::vector<VulkanDescriptorSetLayoutBinding> bindings;
		VkDescriptorSetLayout layout = VK_NULL_HANDLE;

		std::vector<VkDescriptorPoolSize> poolSizes;
		uint32_t maxSetsCount = 0;

		std::function<void(VulkanStorage&, VkDescriptorSetLayout)> allocator;
	};

}