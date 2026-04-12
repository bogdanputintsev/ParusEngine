#pragma once

#include "VulkanRenderPass.h"


namespace parus::vulkan
{

	class VulkanDescriptorManager;

	class MainPass final : public VulkanRenderPass
	{
	public:
		void init(VulkanStorage& storage, const VulkanConfigurator& config) override;
		void initPipelines(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager);
		void record(const FrameContext& frame, const VulkanStorage& storage, const SceneData& scene) const override;
		void cleanup(const VulkanStorage& storage) override;

	private:
		VkPipeline skyPipeline = VK_NULL_HANDLE;
		VkPipelineLayout skyPipelineLayout = VK_NULL_HANDLE;

		void setFullscreenViewportScissor(VkCommandBuffer commandBuffer, const VulkanStorage& storage) const;
		void drawSkyboxPass(const FrameContext& frame, const VulkanStorage& storage) const;
		void drawMainScenePass(const FrameContext& frame, const VulkanStorage& storage, const SceneData& scene) const;
	};

}
