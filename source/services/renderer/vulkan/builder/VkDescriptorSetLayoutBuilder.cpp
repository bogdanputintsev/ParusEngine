#include "VkDescriptorSetLayoutBuilder.h"


namespace parus::vulkan
{
    VkDescriptorSetLayoutBuilder::VkDescriptorSetLayoutBuilder(const std::string& name)
        : debugName(name)
    {
    }

    VkDescriptorSetLayout VkDescriptorSetLayoutBuilder::build(const VulkanStorage& vulkanStorage)
    {
        const VkDescriptorSetLayoutCreateInfo layoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .bindingCount = static_cast<uint32_t>(bindings.size()),
            .pBindings = bindings.data()
        };

        VkDescriptorSetLayout newDescriptorSetLayout;
        ASSERT(vkCreateDescriptorSetLayout(vulkanStorage.logicalDevice, &layoutInfo, nullptr, &newDescriptorSetLayout) == VK_SUCCESS,
               "Failed to create new descriptor set layout " + debugName);

        utils::setDebugName(vulkanStorage, newDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, debugName.c_str());

        return newDescriptorSetLayout;
    }

    VkDescriptorSetLayoutBuilder& VkDescriptorSetLayoutBuilder::addBinding(const VulkanDescriptorSetLayoutBinding& newBinding)
    {
        VkDescriptorSetLayoutBinding newDescriptorSetLayoutBinding =
        {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = newBinding.descriptorType,
            .descriptorCount= 1,
            .stageFlags = newBinding.stageFlags,
            .pImmutableSamplers= nullptr
        };

        bindings.emplace_back(newDescriptorSetLayoutBinding);
        
        return *this;
    }

    VkDescriptorSetLayoutBuilder& VkDescriptorSetLayoutBuilder::withBindings(const std::vector<VulkanDescriptorSetLayoutBinding>& newBindings)
    {
        bindings.reserve(newBindings.size());
        
        for (const VulkanDescriptorSetLayoutBinding& newBinding : newBindings)
        {
            addBinding(newBinding);
        }

        return *this;
    }
}
