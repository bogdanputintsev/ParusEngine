#include "VulkanRenderPass.h"

#include "services/renderer/vulkan/storage/VulkanStorage.h"


namespace parus::vulkan
{

	void VulkanRenderPass::cleanup(const VulkanStorage& storage)
	{
		if (pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(storage.logicalDevice, pipeline, nullptr);
			pipeline = VK_NULL_HANDLE;
		}
		if (pipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(storage.logicalDevice, pipelineLayout, nullptr);
			pipelineLayout = VK_NULL_HANDLE;
		}
		if (framebuffer != VK_NULL_HANDLE)
		{
			vkDestroyFramebuffer(storage.logicalDevice, framebuffer, nullptr);
			framebuffer = VK_NULL_HANDLE;
		}
		if (renderPass != VK_NULL_HANDLE)
		{
			vkDestroyRenderPass(storage.logicalDevice, renderPass, nullptr);
			renderPass = VK_NULL_HANDLE;
		}
	}

	void VulkanRenderPass::onSwapchainRecreate(VulkanStorage& storage, const VulkanConfigurator& config)
	{
		// Default: no-op. Override in resolution-dependent passes.
	}

}
