#include "VulkanGraphicsPipelineManager.h"

#include <memory>
#include <stdexcept>

#include "ShaderLoader.h"

namespace tessera::vulkan
{
	
	void VulkanGraphicsPipelineManager::init(const std::shared_ptr<const VkDevice>& device)
	{
		const auto vertexShaderCode = ShaderLoader::readFile("shaders/vert.spv");
		vertexShaderModule = createShaderModule(vertexShaderCode, device);

		const auto fragmentShaderCode = ShaderLoader::readFile("shaders/frag.spv");
		fragmentShaderModule = createShaderModule(fragmentShaderCode, device);

		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vertexShaderModule;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragmentShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };
	}

	void VulkanGraphicsPipelineManager::clean(const std::shared_ptr<const VkDevice>& device) const
	{
		vkDestroyShaderModule(*device, fragmentShaderModule, nullptr);
		vkDestroyShaderModule(*device, vertexShaderModule, nullptr);
	}

	VkShaderModule VulkanGraphicsPipelineManager::createShaderModule(const std::vector<char>& code, const std::shared_ptr<const VkDevice>& device)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		if (vkCreateShaderModule(*device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) 
		{
			throw std::runtime_error("VulkanGraphicsPipelineManager: failed to create shader module.");
		}

		return shaderModule;
	}
}
