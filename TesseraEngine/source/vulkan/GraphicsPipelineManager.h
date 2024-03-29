#pragma once
#include <memory>
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

		[[nodiscard]] std::shared_ptr<VkRenderPass> getRenderPath() const { return renderPass; }
		[[nodiscard]] VkPipeline getGraphicsPipeline() const { return graphicsPipeline; }

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

