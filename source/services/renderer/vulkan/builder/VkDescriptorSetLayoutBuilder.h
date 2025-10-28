#pragma once

#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    struct VulkanDescriptorSetLayoutBinding final
    {
        [[maybe_unused]] std::string bindingName;
        VkDescriptorType descriptorType;
        VkShaderStageFlags stageFlags;
    };
    
    class VkDescriptorSetLayoutBuilder final
    {
    public:
        explicit VkDescriptorSetLayoutBuilder(std::string name);
        VkDescriptorSetLayout build(const VulkanStorage& vulkanStorage);
        VkDescriptorSetLayoutBuilder& withBindings(const std::vector<VulkanDescriptorSetLayoutBinding>& newBindings);
    private:
        VkDescriptorSetLayoutBuilder& addBinding(const VulkanDescriptorSetLayoutBinding& newBinding);

        std::string debugName;
        std::vector<VkDescriptorSetLayoutBinding> bindings;
    };
}

