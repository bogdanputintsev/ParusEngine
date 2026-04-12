#pragma once

#include "VulkanRenderPass.h"


namespace parus::vulkan
{

	class VulkanDescriptorManager;

	class DepthPrePass final : public VulkanRenderPass
	{
	public:
		void init(VulkanStorage& storage, const VulkanConfigurator& config) override;
		void initPipeline(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager);
		void record(const FrameContext& frame, const VulkanStorage& storage, const SceneData& scene) const override;
		void cleanup(const VulkanStorage& storage) override;
		void onSwapchainRecreate(VulkanStorage& storage, const VulkanConfigurator& config) override;

		[[nodiscard]] VkImageView getImageView() const { return imageView; }
		[[nodiscard]] VkSampler getSampler() const { return sampler; }

	private:
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory imageMemory = VK_NULL_HANDLE;
		VkImageView imageView = VK_NULL_HANDLE;
		VkSampler sampler = VK_NULL_HANDLE;
	};

}
