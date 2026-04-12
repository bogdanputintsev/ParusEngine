#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "services/renderer/vulkan/mesh/MeshInstance.h"
#include "services/renderer/vulkan/light/Light.h"


namespace parus::vulkan
{

	struct VulkanStorage;
	struct VulkanConfigurator;
	class VulkanDescriptorManager;

	struct FrameContext
	{
		VkCommandBuffer commandBuffer;
		uint32_t currentFrame;
		uint32_t imageIndex;
	};

	struct SceneData
	{
		const std::vector<MeshInstance>& meshInstances;
		const VulkanDirectionalLight& directionalLight;
		const std::vector<PointLight>& pointLights;
	};

	class VulkanRenderPass
	{
	public:
		virtual ~VulkanRenderPass() = default;

		virtual void init(VulkanStorage& storage, const VulkanConfigurator& config) = 0;
		virtual void record(const FrameContext& frame, const VulkanStorage& storage, const SceneData& scene) const = 0;
		virtual void cleanup(const VulkanStorage& storage);
		virtual void onSwapchainRecreate(VulkanStorage& storage, const VulkanConfigurator& config);

	protected:
		VkRenderPass renderPass = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkFramebuffer framebuffer = VK_NULL_HANDLE;
	};

}
