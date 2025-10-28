#include "VkDescriptorPoolBuilder.h"


namespace parus::vulkan
{
    void VkDescriptorPoolBuilder::build(VulkanStorage& storage) const
    {
        ASSERT(!poolSizes.empty(), "Missing information about pool sizes for descriptor pool builder.");

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = maxSets;

        ASSERT(vkCreateDescriptorPool(storage.logicalDevice, &poolInfo, nullptr, &storage.descriptorPool) == VK_SUCCESS, "failed to create descriptor pool.");
    }

    VkDescriptorPoolBuilder& VkDescriptorPoolBuilder::setMaxSets(const uint32_t newMaxSets)
    {
        maxSets = newMaxSets;

        ASSERT(maxSets > 0, "Max number of sets can not be 0.");

        return *this;
    }

    VkDescriptorPoolBuilder& VkDescriptorPoolBuilder::setPoolSizes(const std::vector<VkDescriptorPoolSize>& newPoolSizes)
    {
        poolSizes = newPoolSizes;
        return *this;
    }
    
}
