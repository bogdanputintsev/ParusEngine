#include "VulkanDescriptor.h"

namespace parus::vulkan
{

	VulkanDescriptor& VulkanDescriptor::withLayout(std::string name,
	                                               std::vector<VulkanDescriptorSetLayoutBinding> bindings_)
	{
		layoutName = std::move(name);
		bindings = std::move(bindings_);
		return *this;
	}

	VulkanDescriptor& VulkanDescriptor::withPoolSizes(std::vector<VkDescriptorPoolSize> sizes, const uint32_t maxSets)
	{
		poolSizes = std::move(sizes);
		maxSetsCount = maxSets;
		return *this;
	}

	VulkanDescriptor& VulkanDescriptor::withAllocator(std::function<void(VulkanStorage&, VkDescriptorSetLayout)> fn)
	{
		allocator = std::move(fn);
		return *this;
	}

}