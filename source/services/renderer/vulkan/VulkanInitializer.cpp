#include "VulkanInitializer.h"

#include "builder/VkDebugUtilsBuilder.h"
#include "builder/VkDeviceFactory.h"
#include "builder/VkFramebufferBuilder.h"
#include "builder/VkInstanceBuilder.h"
#include "builder/VkQueuesFactory.h"
#include "builder/VkRenderPassFactory.h"
#include "builder/VkSurfaceFactory.h"
#include "builder/VkSwapChainFactory.h"
#include "builder/VkSyncObjectsFactory.h"
#include "builder/VkUboBuilder.h"
#include "builder/VulkanTexture2dBuilder.h"
#include "engine/utils/math/UniformBufferObjects.h"
#include "services/Services.h"
#include "services/config/Configs.h"
#include "services/graphics/GraphicsLibrary.h"
#include "services/world/World.h"
#include "utils/VulkanUtils.h"
#include "VulkanRenderer.h"
#include "builder/VkCommandBufferBuilder.h"

#include <array>

namespace parus::vulkan
{
	
	void VulkanInitializer::initialize(VulkanStorage& storage, VulkanDescriptorManager& descriptorManager, const VulkanConfigurator& configurator)
	{
		VkInstanceBuilder()
			.setApplicationName(Services::get<Configs>()->get("Engine", "applicationName"))
			.setVersion(
				Services::get<Configs>()->getAsInt("Engine", "versionMajor").value_or(0),
				Services::get<Configs>()->getAsInt("Engine", "versionMinor").value_or(0),
				Services::get<Configs>()->getAsInt("Engine", "versionPatch").value_or(0))
			.setDebugCallback(VkDebugUtilsBuilder::debugCallback)
			.setRequiredExtensions(getRequiredExtensions())
			.setValidationLayers({"VK_LAYER_KHRONOS_validation"})
			.build(storage);

		VkDebugUtilsBuilder()
			.setDebugCallback(VkDebugUtilsBuilder::debugCallback)
			.build(storage);

		VkSurfaceFactory::build(storage);
		VkDeviceFactory::build(storage);
		VkQueuesFactory::build(storage);
		createSwapChain(storage);
		VkRenderPassFactory::build("Main Render Pass", storage);
		descriptorManager.setup(storage);
		// Sky and main pipelines are now created by MainPass
		createColorResources(storage);
		createDepthResources(storage);
		createFramebuffers(storage);
		createUniformBuffer(storage);

		// Shadow map and SSAO resources are now initialized by their pass classes
		// (called from VulkanRenderer::init after this method returns)

		VkSyncObjectsFactory().build(storage);
		
		storage.commandBuffers = VkCommandBufferBuilder()
			.setCommandPool(utils::getCommandPool(storage))
			.setCount(VulkanStorage::MAX_FRAMES_IN_FLIGHT)
			.build("Main Command Buffer", storage);
	}

	void VulkanInitializer::cleanup(VulkanStorage& storage, VulkanDescriptorManager& descriptorManager)
	{
		cleanupSwapChain(storage);

		// Main and sky pipeline cleanup is now handled by MainPass::cleanup()
		vkDestroyRenderPass(storage.logicalDevice, storage.renderPass, nullptr);

		// Shadow map and SSAO cleanup is now handled by their pass classes

		for (size_t i = 0; i < VulkanStorage::MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(storage.logicalDevice, storage.globalUboBuffer.frameBuffers[i], nullptr);
			vkDestroyBuffer(storage.logicalDevice, storage.instanceUboBuffer.frameBuffers[i], nullptr);
			vkDestroyBuffer(storage.logicalDevice, storage.directionalLightUboBuffer.frameBuffers[i], nullptr);
			vkDestroyBuffer(storage.logicalDevice, storage.pointLightUboBuffer.frameBuffers[i], nullptr);
			vkFreeMemory(storage.logicalDevice, storage.globalUboBuffer.memory[i], nullptr);
			vkFreeMemory(storage.logicalDevice, storage.instanceUboBuffer.memory[i], nullptr);
			vkFreeMemory(storage.logicalDevice, storage.directionalLightUboBuffer.memory[i], nullptr);
			vkFreeMemory(storage.logicalDevice, storage.pointLightUboBuffer.memory[i], nullptr);
		}

		descriptorManager.cleanup(storage);
		cleanupTextures(storage);

		vkDestroyBuffer(storage.logicalDevice, storage.globalBuffers.indexBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, storage.globalBuffers.indexBufferMemory, nullptr);

		vkDestroyBuffer(storage.logicalDevice, storage.globalBuffers.vertexBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, storage.globalBuffers.vertexBufferMemory, nullptr);

		vkDestroyBuffer(storage.logicalDevice, storage.globalBuffers.skyIndexBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, storage.globalBuffers.skyIndexBufferMemory, nullptr);

		vkDestroyBuffer(storage.logicalDevice, storage.globalBuffers.skyVertexBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, storage.globalBuffers.skyVertexBufferMemory, nullptr);

		for (size_t i = 0; i < VulkanStorage::MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(storage.logicalDevice, storage.renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(storage.logicalDevice, storage.imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(storage.logicalDevice, storage.inFlightFences[i], nullptr);
		}

		for (const auto& [_, commandPool] : storage.threadCommandPools)
		{
			vkDestroyCommandPool(storage.logicalDevice, commandPool, nullptr);
		}

		vkDestroyDevice(storage.logicalDevice, nullptr);

		VkDebugUtilsBuilder::destroy(storage);

		vkDestroySurfaceKHR(storage.instance, storage.surface, nullptr);
		vkDestroyInstance(storage.instance, nullptr);
	}

	void VulkanInitializer::recreateSwapChain(VulkanStorage& storage)
	{
		Services::get<GraphicsLibrary>()->handleMinimization();
		Services::get<Renderer>()->deviceWaitIdle();

		cleanupSwapChain(storage);

		createSwapChain(storage);
		createColorResources(storage);
		createDepthResources(storage);
		createFramebuffers(storage);
	}

	void VulkanInitializer::createSwapChain(VulkanStorage& storage)
	{
		VkSwapChainFactory::build(storage);
	}

	void VulkanInitializer::cleanupSwapChain(const VulkanStorage& storage) const
	{
		vkDestroyImageView(storage.logicalDevice, depthTexture.imageView, nullptr);
		vkDestroyImage(storage.logicalDevice, depthTexture.image, nullptr);
		vkFreeMemory(storage.logicalDevice, depthTexture.imageMemory, nullptr);

		vkDestroyImageView(storage.logicalDevice, colorTexture.imageView, nullptr);
		vkDestroyImage(storage.logicalDevice, colorTexture.image, nullptr);
		vkFreeMemory(storage.logicalDevice, colorTexture.imageMemory, nullptr);

		for (const auto framebuffer : storage.swapChainFramebuffers)
		{
			vkDestroyFramebuffer(storage.logicalDevice, framebuffer, nullptr);
		}

		for (const auto imageView : storage.swapChainDetails.swapChainImageViews)
		{
			vkDestroyImageView(storage.logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(storage.logicalDevice, storage.swapChain, nullptr);
	}


	void VulkanInitializer::createFramebuffers(VulkanStorage& storage) const
	{
		storage.swapChainFramebuffers.resize(storage.swapChainDetails.swapChainImageViews.size());

		for (size_t i = 0; i < storage.swapChainDetails.swapChainImageViews.size(); ++i)
		{
			storage.swapChainFramebuffers[i] = VkFramebufferBuilder()
				.setColorImageView(colorTexture.imageView)
				.setDepthImageView(depthTexture.imageView)
				.setSwapChainImageView(storage.swapChainDetails.swapChainImageViews[i])
				.build("Framebuffer " + std::to_string(i), storage);
		}
	}

	void VulkanInitializer::createDepthResources(const VulkanStorage& storage)
	{
		depthTexture = VulkanTexture2dBuilder("Depth Texture")
			.setWidth(storage.swapChainDetails.swapChainExtent.width)
			.setHeight(storage.swapChainDetails.swapChainExtent.height)
			.setNumberOfSamples(storage.msaaSamples)
			.setFormat(utils::findDepthFormat(storage))
			.setAspectMask(VK_IMAGE_ASPECT_DEPTH_BIT)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);
	}

	void VulkanInitializer::createColorResources(const VulkanStorage& storage)
	{
		colorTexture = VulkanTexture2dBuilder("Color Texture")
			.setWidth(storage.swapChainDetails.swapChainExtent.width)
			.setHeight(storage.swapChainDetails.swapChainExtent.height)
			.setNumberOfSamples(storage.msaaSamples)
			.setFormat(storage.swapChainDetails.swapChainImageFormat)
			.setAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.setUsage(VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);
	}

	void VulkanInitializer::createUniformBuffer(VulkanStorage& storage)
	{
		storage.globalUboBuffer = VkUboBuilder()
			.setSize(sizeof(math::GlobalUbo))
			.build("Global UBO", storage);

		storage.instanceUboBuffer = VkUboBuilder()
			.setSize(sizeof(math::InstanceUbo))
			.build("Instance UBO", storage);

		storage.directionalLightUboBuffer = VkUboBuilder()
			.setSize(sizeof(math::DirectionalLightUbo))
			.build("Directional Light UBO", storage);

		storage.pointLightUboBuffer = VkUboBuilder()
			.setSize(sizeof(math::PointLightUbo))
			.build("Point Light UBO", storage);
	}

	void VulkanInitializer::cleanupTextures(const VulkanStorage& storage)
	{
		for (const auto& texture : Services::get<World>()->getStorage()->getAllTextures())
		{
			auto* vulkanTexture = dynamic_cast<VulkanTexture2d*>(texture.get());
			ASSERT(vulkanTexture, "Expected VulkanTexture2d in Vulkan renderer cleanup.");
			if (vulkanTexture->sampler)
			{
				vkDestroySampler(storage.logicalDevice, vulkanTexture->sampler, nullptr);
				vulkanTexture->sampler = nullptr;
			}
			if (vulkanTexture->imageView)
			{
				vkDestroyImageView(storage.logicalDevice, vulkanTexture->imageView, nullptr);
				vulkanTexture->imageView = nullptr;
			}
			if (vulkanTexture->image)
			{
				vkDestroyImage(storage.logicalDevice, vulkanTexture->image, nullptr);
				vulkanTexture->image = nullptr;
			}
			if (vulkanTexture->imageMemory)
			{
				vkFreeMemory(storage.logicalDevice, vulkanTexture->imageMemory, nullptr);
				vulkanTexture->imageMemory = nullptr;
			}
		}
	}

	VkSampleCountFlagBits VulkanInitializer::getMaxUsableSampleCount(const VulkanStorage& storage)
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(storage.physicalDevice, &physicalDeviceProperties);

		const VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT)  { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT)  { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT)  { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}

	std::vector<const char*> VulkanInitializer::getRequiredExtensions()
	{
		std::vector extensions = Services::get<GraphicsLibrary>()->getRequiredExtensions();

		if (VkInstanceBuilder::validationLayersAreEnabled())
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}


	// createSSAOResources and cleanupSSAOResources have been moved to their respective pass classes:
	// DepthPrePass, SSAOPass, SSAOBlurPass


}
