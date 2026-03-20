#include "VulkanDescriptorManager.h"

#include "builder/VkDescriptorPoolBuilder.h"
#include "builder/VkDescriptorSetLayoutBuilder.h"
#include "engine/utils/Utils.h"
#include "storage/VulkanStorage.h"

namespace parus::vulkan
{

	void VulkanDescriptorManager::define(const DescriptorType type, VulkanDescriptor descriptor)
	{
		descriptors.emplace(type, std::move(descriptor));
	}

	void VulkanDescriptorManager::setup(VulkanStorage& storage)
	{
		uint32_t totalMaxSets = 0;
		std::vector<VkDescriptorPoolSize> allPoolSizes;

		for (auto& [type, descriptor] : descriptors)
		{
			descriptor.layout = VkDescriptorSetLayoutBuilder(descriptor.layoutName)
				.withBindings(descriptor.bindings)
				.build(storage);

			totalMaxSets += descriptor.maxSetsCount;
			for (const auto& ps : descriptor.poolSizes)
				allPoolSizes.push_back(ps);
		}

		VkDescriptorPoolBuilder()
			.setMaxSets(totalMaxSets)
			.setPoolSizes(allPoolSizes)
			.build(storage);
	}

	void VulkanDescriptorManager::cleanup(const VulkanStorage& storage)
	{
		vkDestroyDescriptorPool(storage.logicalDevice, storage.descriptorPool, nullptr);

		for (auto& [type, desc] : descriptors)
		{
			if (desc.layout != VK_NULL_HANDLE)
				vkDestroyDescriptorSetLayout(storage.logicalDevice, desc.layout, nullptr);
		}
	}

	void VulkanDescriptorManager::rebuildAll(VulkanStorage& storage)
	{
		for (auto& [type, desc] : descriptors)
		{
			if (desc.allocator)
				desc.allocator(storage, desc.layout);
		}
	}

	VkDescriptorSetLayout VulkanDescriptorManager::getLayout(const DescriptorType type) const
	{
		const auto it = descriptors.find(type);
		ASSERT(it != descriptors.end(), "Descriptor type not registered in VulkanDescriptorManager.");
		return it->second.layout;
	}

}