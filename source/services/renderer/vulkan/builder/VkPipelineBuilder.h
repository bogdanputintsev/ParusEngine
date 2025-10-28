#pragma once
#include <vector>

#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    
    class VkPipelineBuilder final
    {
    public:
        explicit VkPipelineBuilder(std::string name);

        void build(const VulkanStorage& storage, VkPipelineLayout& outPipelineLayout, VkPipeline& outPipeline);

        VkPipelineBuilder& addStage(const VulkanStorage& storage, VkShaderStageFlagBits stageType, const std::string& shaderPath);
        VkPipelineBuilder& useLayouts(const std::vector<VkDescriptorSetLayout>& layouts);

        VkPipelineBuilder& withVertexInput(
            const std::vector<VkVertexInputBindingDescription>& bindings,
            const std::vector<VkVertexInputAttributeDescription>& attributes);
        VkPipelineBuilder& withInputAssembly();
        VkPipelineBuilder& withViewportState();
        VkPipelineBuilder& withRasterization();
        VkPipelineBuilder& withMultisample(VkSampleCountFlagBits rasterizationSamples);
        VkPipelineBuilder& withDepthStencil(bool depthWriteEnabled);
        VkPipelineBuilder& withColorBlend();
        VkPipelineBuilder& withDynamicState();
        
    private:
        std::string debugName;
        
        std::vector<VkPipelineShaderStageCreateInfo> pipelineStages;
        std::vector<VkDescriptorSetLayout> pipelineLayouts;
        std::list<VkShaderModule> allShaderModules;

        std::optional<VkPipelineVertexInputStateCreateInfo> vertexInputState;
        std::optional<VkPipelineInputAssemblyStateCreateInfo> inputAssemblyState;
        std::optional<VkPipelineViewportStateCreateInfo> viewportState;
        std::optional<VkPipelineRasterizationStateCreateInfo> rasterizationState;
        std::optional<VkPipelineMultisampleStateCreateInfo> multisampleState;
        std::optional<VkPipelineDepthStencilStateCreateInfo> depthStencilState;
        std::optional<VkPipelineColorBlendStateCreateInfo> colorBlendState;
        std::optional<VkPipelineDynamicStateCreateInfo> dynamicState;

        void cleanShaderModules(const VulkanStorage& storage);
        static VkShaderModule createShaderModule(const VulkanStorage& storage, const std::string& shaderPath);
    };
}
