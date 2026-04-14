#pragma once

#include <vector>
#include <vulkan/vulkan_core.h>

#include "storage/VulkanStorage.h"
#include "texture/VulkanTexture2d.h"
#include "VulkanConfigurator.h"
#include "VulkanDescriptorManager.h"


namespace parus::vulkan
{

	class VulkanInitializer final
	{
	public:
		void initialize(VulkanStorage& storage, VulkanDescriptorManager& descriptorManager, const VulkanConfigurator& configurator);
		void cleanup(VulkanStorage& storage, VulkanDescriptorManager& descriptorManager);
		void recreateSwapChain(VulkanStorage& storage);

	private:
		VulkanTexture2d colorTexture;
		VulkanTexture2d depthTexture;

		static void createSwapChain(VulkanStorage& storage);
		void cleanupSwapChain(const VulkanStorage& storage) const;
		void createFramebuffers(VulkanStorage& storage) const;
		void createDepthResources(const VulkanStorage& storage);
		void createColorResources(const VulkanStorage& storage);
		static void createUniformBuffer(VulkanStorage& storage);
		static void cleanupTextures(const VulkanStorage& storage);

		[[nodiscard]] static VkSampleCountFlagBits getMaxUsableSampleCount(const VulkanStorage& storage);
		[[nodiscard]] static std::vector<const char*> getRequiredExtensions();
	};

}