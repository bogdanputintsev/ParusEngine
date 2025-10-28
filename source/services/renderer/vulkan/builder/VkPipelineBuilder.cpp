#include "VkPipelineBuilder.h"

#include "engine/utils/Utils.h"
#include "engine/utils/math/Math.h"
#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{
    VkPipelineBuilder::VkPipelineBuilder(std::string name)
       : debugName(std::move(name))
    {
    }

    void VkPipelineBuilder::build(const VulkanStorage& storage, VkPipelineLayout& outPipelineLayout, VkPipeline& outPipeline)
    {
        // Create Pipeline Layout
        const VkPipelineLayoutCreateInfo pipelineLayoutInfo =
        {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .setLayoutCount = static_cast<uint32_t>(pipelineLayouts.size()),
            .pSetLayouts = pipelineLayouts.data(),
            .pushConstantRangeCount = 0,
            .pPushConstantRanges = nullptr,
        };

        ASSERT(vkCreatePipelineLayout(storage.logicalDevice, &pipelineLayoutInfo, nullptr, &outPipelineLayout) == VK_SUCCESS,
            "Failed to create pipeline layout " + debugName);
        
        utils::setDebugName(storage, outPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT, std::string(debugName + " Layout").c_str());
        
        // Create Pipeline 
        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = static_cast<uint32_t>(pipelineStages.size());
        pipelineInfo.pStages = pipelineStages.data();
        pipelineInfo.pVertexInputState = vertexInputState.has_value() ? &*vertexInputState : nullptr;
        pipelineInfo.pInputAssemblyState = inputAssemblyState.has_value() ? &*inputAssemblyState : nullptr;
        pipelineInfo.pViewportState = viewportState.has_value() ? &*viewportState : nullptr;
        pipelineInfo.pRasterizationState = rasterizationState.has_value() ? &*rasterizationState : nullptr;
        pipelineInfo.pMultisampleState = multisampleState.has_value() ? &*multisampleState : nullptr;
        pipelineInfo.pDepthStencilState = depthStencilState.has_value() ? &*depthStencilState : nullptr;
        pipelineInfo.pColorBlendState = colorBlendState.has_value() ? &*colorBlendState : nullptr;
        pipelineInfo.pDynamicState = dynamicState.has_value() ? &*dynamicState : nullptr;
        pipelineInfo.layout = outPipelineLayout;
        pipelineInfo.renderPass = storage.renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        ASSERT(vkCreateGraphicsPipelines(storage.logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &outPipeline) == VK_SUCCESS,
            "Failed to create pipeline " + debugName);

        utils::setDebugName(storage, outPipeline, VK_OBJECT_TYPE_PIPELINE, debugName.c_str());

        // Clean shader modules because we don't need them anymore.
        cleanShaderModules(storage);
    }

    void VkPipelineBuilder::cleanShaderModules(const VulkanStorage& storage)
    {
        for (const VkShaderModule& shaderModule : allShaderModules)
        {
            vkDestroyShaderModule(storage.logicalDevice, shaderModule, nullptr);
        }
        
        allShaderModules.clear();
    }

    VkPipelineBuilder& VkPipelineBuilder::addStage(const VulkanStorage& storage, const VkShaderStageFlagBits stageType, const std::string& shaderPath)
    {
        const VkShaderModule shaderModule = createShaderModule(storage, shaderPath);
        allShaderModules.push_back(shaderModule);

        VkPipelineShaderStageCreateInfo newPipelineShaderStageInfo{};
        newPipelineShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        newPipelineShaderStageInfo.stage = stageType;
        newPipelineShaderStageInfo.module = shaderModule;
        newPipelineShaderStageInfo.pName = "main";

        pipelineStages.push_back(newPipelineShaderStageInfo);

        return *this;
    }

    VkShaderModule VkPipelineBuilder::createShaderModule(const VulkanStorage& storage, const std::string& shaderPath)
    {
        const auto shaderCode = ::parus::utils::readFile(shaderPath);
        
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = shaderCode.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(shaderCode.data());

        VkShaderModule shaderModule;
        ASSERT(vkCreateShaderModule(storage.logicalDevice, &createInfo, nullptr, &shaderModule) == VK_SUCCESS, "failed to create shader module.");
        utils::setDebugName(storage, shaderModule, VK_OBJECT_TYPE_SHADER_MODULE, std::string("Shader module " + shaderPath).c_str());
    
        return shaderModule;
    }
    
    VkPipelineBuilder& VkPipelineBuilder::useLayouts(const std::vector<VkDescriptorSetLayout>& layouts)
    {
        pipelineLayouts = layouts;
        return *this;
    }

    VkPipelineBuilder& VkPipelineBuilder::withVertexInput(const std::vector<VkVertexInputBindingDescription>& bindings, const std::vector<VkVertexInputAttributeDescription>& attributes)
    {
        // Vertex input
        VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo{};
        vertexInputStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputStateCreateInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
        vertexInputStateCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
        vertexInputStateCreateInfo.pVertexBindingDescriptions = bindings.data();
        vertexInputStateCreateInfo.pVertexAttributeDescriptions = attributes.data();

        vertexInputState = vertexInputStateCreateInfo;

        return *this;
    }

    VkPipelineBuilder& VkPipelineBuilder::withInputAssembly()
    {
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo{};
        inputAssemblyStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssemblyStateCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssemblyStateCreateInfo.primitiveRestartEnable = VK_FALSE;

        inputAssemblyState = inputAssemblyStateCreateInfo;

        return *this;
    }

    VkPipelineBuilder& VkPipelineBuilder::withViewportState()
    {
        VkPipelineViewportStateCreateInfo viewportStateCreateInfo{};
        viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportStateCreateInfo.viewportCount = 1;
        viewportStateCreateInfo.scissorCount = 1;

        viewportState = viewportStateCreateInfo;

        return *this;
    }

    VkPipelineBuilder& VkPipelineBuilder::withRasterization()
    {
        VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo{};
        rasterizationStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizationStateCreateInfo.depthClampEnable = VK_FALSE;
        rasterizationStateCreateInfo.rasterizerDiscardEnable = VK_FALSE;
        rasterizationStateCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizationStateCreateInfo.lineWidth = 1.0f;
        rasterizationStateCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizationStateCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizationStateCreateInfo.depthBiasEnable = VK_FALSE;
        rasterizationStateCreateInfo.depthBiasConstantFactor = 0.0f;
        rasterizationStateCreateInfo.depthBiasClamp = 0.0f;
        rasterizationStateCreateInfo.depthBiasSlopeFactor = 0.0f;

        rasterizationState = rasterizationStateCreateInfo;

        return *this;
    }

    VkPipelineBuilder& VkPipelineBuilder::withMultisample(const VkSampleCountFlagBits rasterizationSamples)
    {
        VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo{};
        multisampleStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampleStateCreateInfo.sampleShadingEnable = VK_FALSE;
        multisampleStateCreateInfo.minSampleShading = 1.0f;
        multisampleStateCreateInfo.rasterizationSamples = rasterizationSamples;
        multisampleStateCreateInfo.pSampleMask = nullptr;
        multisampleStateCreateInfo.alphaToCoverageEnable = VK_FALSE;
        multisampleStateCreateInfo.alphaToOneEnable = VK_FALSE;

        multisampleState = multisampleStateCreateInfo;

        return *this;
    }

    VkPipelineBuilder& VkPipelineBuilder::withDepthStencil(const bool depthWriteEnabled)
    {
        VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo{};
        depthStencilStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencilStateCreateInfo.depthTestEnable = VK_TRUE;
        depthStencilStateCreateInfo.depthWriteEnable = depthWriteEnabled ? VK_TRUE : VK_FALSE;
        depthStencilStateCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencilStateCreateInfo.depthBoundsTestEnable = VK_FALSE;
        depthStencilStateCreateInfo.stencilTestEnable = VK_FALSE;

        depthStencilState = depthStencilStateCreateInfo;

        return *this;
    }

    VkPipelineBuilder& VkPipelineBuilder::withColorBlend()
    {
        // Color blending.
        static constexpr VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo{};
        colorBlendStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlendStateCreateInfo.logicOpEnable = VK_FALSE;
        colorBlendStateCreateInfo.logicOp = VK_LOGIC_OP_COPY;
        colorBlendStateCreateInfo.attachmentCount = 1;
        colorBlendStateCreateInfo.pAttachments = &colorBlendAttachment;
        colorBlendStateCreateInfo.blendConstants[0] = 0.0f;
        colorBlendStateCreateInfo.blendConstants[1] = 0.0f;
        colorBlendStateCreateInfo.blendConstants[2] = 0.0f;
        colorBlendStateCreateInfo.blendConstants[3] = 0.0f;

        colorBlendState = colorBlendStateCreateInfo;

        return *this;
    }

    VkPipelineBuilder& VkPipelineBuilder::withDynamicState()
    {
        static constexpr std::array DYNAMIC_STATES = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };

        VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo{};
        dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(DYNAMIC_STATES.size());
        dynamicStateCreateInfo.pDynamicStates = DYNAMIC_STATES.data();

        dynamicState = dynamicStateCreateInfo;

        return *this;
    }
}
