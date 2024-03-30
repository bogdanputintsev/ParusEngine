#pragma once
#include <vector>
#include <vulkan/vulkan_core.h>
#include "SwapChainManager.h"

namespace tessera::vulkan
{

	class GraphicsPipelineManager final : public Initializable
	{
	public:
		void init() override;
		void clean() override;

		[[nodiscard]] VkRenderPass getRenderPath() const { return renderPass; }
		[[nodiscard]] VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }

	private:
		static VkShaderModule createShaderModule(const std::vector<char>& code, const VkDevice& device);
		void initRenderPath(const VkDevice& device, const SwapChainImageDetails& swapChainImageDetails);

		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline graphicsPipeline = VK_NULL_HANDLE;
		VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
	};
	
}

