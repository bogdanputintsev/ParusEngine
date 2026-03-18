#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "storage/VulkanStorage.h"
#include "texture/VulkanTexture2d.h"


namespace parus::vulkan
{

	class VulkanInitializer final
	{
	public:
		void initialize(VulkanStorage& storage);
		void cleanup(VulkanStorage& storage);
		void recreateSwapChain(VulkanStorage& storage);

	private:
		VulkanTexture2d colorTexture;
		VulkanTexture2d depthTexture;

		static void createSwapChain(VulkanStorage& storage);
		void cleanupSwapChain(const VulkanStorage& storage) const;
		static void createDescriptorSetLayout(VulkanStorage& storage);
		static void createDescriptorPool(VulkanStorage& storage);
		static void createSkyPipeline(VulkanStorage& storage);
		static void createGraphicsPipeline(VulkanStorage& storage);
		void createFramebuffers(VulkanStorage& storage) const;
		void createDepthResources(const VulkanStorage& storage);
		void createColorResources(const VulkanStorage& storage);
		static void createUniformBuffer(VulkanStorage& storage);
		static void cleanupTextures(const VulkanStorage& storage);

		[[nodiscard]] static VkSampleCountFlagBits getMaxUsableSampleCount(const VulkanStorage& storage);
		[[nodiscard]] static std::vector<const char*> getRequiredExtensions();
	};

}