#pragma once
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>
#include "VulkanSwapChainManager.h"

namespace tessera::vulkan
{

	class VulkanGraphicsPipelineManager
	{
	public:
		void init(const std::shared_ptr<const VkDevice>& device, const SwapChainImageDetails& swapChainImageDetails);
		void clean(const std::shared_ptr<const VkDevice>& device) const;

		[[nodiscard]] std::shared_ptr<VkRenderPass> getRenderPath() const { return renderPass; }
	private:
		static VkShaderModule createShaderModule(const std::vector<char>& code, const std::shared_ptr<const VkDevice>& device);
		void initRenderPath(const std::shared_ptr<const VkDevice>& device, const SwapChainImageDetails& swapChainImageDetails);

		std::shared_ptr<VkRenderPass> renderPass;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;
		VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
	};
	
}

