#include "VulkanInitializer.h"

#include "builder/VkDebugUtilsBuilder.h"
#include "builder/VkDeviceBuilder.h"
#include "builder/VkFramebufferBuilder.h"
#include "builder/VkInstanceBuilder.h"
#include "builder/VkPipelineBuilder.h"
#include "builder/VkQueuesBuilder.h"
#include "builder/VkRenderPassBuilder.h"
#include "builder/VkSurfaceBuilder.h"
#include "builder/VkSwapChainBuilder.h"
#include "builder/VkSyncObjectsBuilder.h"
#include "builder/VkUboBuilder.h"
#include "builder/VkImageBuilder.h"
#include "builder/VkImageViewBuilder.h"
#include "builder/VkDeviceMemoryBuilder.h"
#include "builder/VulkanTexture2dBuilder.h"
#include "engine/utils/math/UniformBufferObjects.h"
#include "services/Services.h"
#include "services/config/Configs.h"
#include "services/graphics/imgui/ImGuiLibrary.h"
#include "services/world/World.h"
#include "utils/VulkanUtils.h"
#include "VulkanRenderer.h"
#include "builder/VkCommandBufferBuilder.h"
#include "builder/VkSamplerBuilder.h"

#include <array>

namespace parus::vulkan
{
	
	void VulkanInitializer::initialize(VulkanStorage& storage, VulkanDescriptorManager& descriptorManager)
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

		VkSurfaceBuilder::build(storage);
		VkDeviceBuilder::build(storage);
		VkQueuesBuilder::build(storage);
		createSwapChain(storage);
		VkRenderPassBuilder::build("Main Render Pass", storage);
		descriptorManager.setup(storage);
		createSkyPipeline(storage, descriptorManager);
		createGraphicsPipeline(storage, descriptorManager);
		createColorResources(storage);
		createDepthResources(storage);
		createFramebuffers(storage);
		createUniformBuffer(storage);

		createShadowMapResources(storage, descriptorManager);
		createSSAOResources(storage, descriptorManager);

		VkSyncObjectsBuilder().build(storage);
		
		storage.commandBuffers = VkCommandBufferBuilder()
			.setCommandPool(utils::getCommandPool(storage))
			.setCount(VulkanStorage::MAX_FRAMES_IN_FLIGHT)
			.build("Main Command Buffer", storage);
	}

	void VulkanInitializer::cleanup(VulkanStorage& storage, VulkanDescriptorManager& descriptorManager)
	{
		cleanupSwapChain(storage);

		vkDestroyPipeline(storage.logicalDevice, storage.mainPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.mainPipelineLayout, nullptr);
		vkDestroyPipeline(storage.logicalDevice, storage.skyPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.skyPipelineLayout, nullptr);
		vkDestroyRenderPass(storage.logicalDevice, storage.renderPass, nullptr);

		// SSAO cleanup
		cleanupSSAOResources(storage);

		// Shadow map cleanup
		vkDestroyPipeline(storage.logicalDevice, storage.shadowPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.shadowPipelineLayout, nullptr);
		vkDestroyRenderPass(storage.logicalDevice, storage.shadowRenderPass, nullptr);
		vkDestroyFramebuffer(storage.logicalDevice, storage.shadowMap.framebuffer, nullptr);
		vkDestroySampler(storage.logicalDevice, storage.shadowMap.sampler, nullptr);
		vkDestroyImageView(storage.logicalDevice, storage.shadowMap.imageView, nullptr);
		vkDestroyImage(storage.logicalDevice, storage.shadowMap.image, nullptr);
		vkFreeMemory(storage.logicalDevice, storage.shadowMap.imageMemory, nullptr);

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

	void VulkanInitializer::recreateSwapChain(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager)
	{
		Services::get<imgui::ImGuiLibrary>()->handleMinimization();
		Services::get<VulkanRenderer>()->deviceWaitIdle();

		cleanupSwapChain(storage);
		cleanupSSAOResources(storage);

		createSwapChain(storage);
		createColorResources(storage);
		createDepthResources(storage);
		createFramebuffers(storage);
		createSSAOResources(storage, descriptorManager);
	}

	void VulkanInitializer::createSwapChain(VulkanStorage& storage)
	{
		VkSwapChainBuilder::build(storage);
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

	void VulkanInitializer::createSkyPipeline(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager)
	{
		VkPipelineBuilder("Sky Pipeline Layout")
			.useLayouts({descriptorManager.getLayout(VulkanDescriptorManager::DescriptorType::GLOBAL)})
			.addStage(storage, VK_SHADER_STAGE_VERTEX_BIT, "bin/shaders/sky.vert.spv")
			.addStage(storage, VK_SHADER_STAGE_FRAGMENT_BIT, "bin/shaders/sky.frag.spv")
			.withVertexInput(
				{
					{
						.binding = 0,
						.stride = sizeof(math::Vertex),
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
					}
				},
				{
					{
						.location = 0,
						.binding = 0,
						.format = VK_FORMAT_R32G32B32_SFLOAT,
						.offset = offsetof(math::Vertex, position)
					}
				})
			.withInputAssembly()
			.withViewportState()
			.withRasterization()
			.withMultisample(storage.msaaSamples)
			.withDepthStencil(false)
			.withColorBlend()
			.withDynamicState()
			.build(storage, storage.skyPipelineLayout, storage.skyPipeline);
	}

	void VulkanInitializer::createGraphicsPipeline(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager)
	{
		using DescriptorType = VulkanDescriptorManager::DescriptorType;
		VkPipelineBuilder("Main Pipeline Layout")
			.useLayouts(
				{
					descriptorManager.getLayout(DescriptorType::GLOBAL),
					descriptorManager.getLayout(DescriptorType::INSTANCE),
					descriptorManager.getLayout(DescriptorType::MATERIAL),
					descriptorManager.getLayout(DescriptorType::LIGHTS),
				})
			.addStage(storage, VK_SHADER_STAGE_VERTEX_BIT, "bin/shaders/main.vert.spv")
			.addStage(storage, VK_SHADER_STAGE_FRAGMENT_BIT, "bin/shaders/main.frag.spv")
			.withVertexInput(
				{
					{
						.binding = 0,
						.stride = sizeof(math::Vertex),
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
					}
				},
				{
					{
						.location = 0,
						.binding = 0,
						.format = VK_FORMAT_R32G32B32_SFLOAT,
						.offset = offsetof(math::Vertex, position)
					},
					{
						.location = 1,
						.binding = 0,
						.format = VK_FORMAT_R32G32B32_SFLOAT,
						.offset = offsetof(math::Vertex, normal)
					},
					{
						.location = 2,
						.binding = 0,
						.format = VK_FORMAT_R32G32B32_SFLOAT,
						.offset = offsetof(math::Vertex, textureCoordinates)
					},
					{
						.location = 3,
						.binding = 0,
						.format = VK_FORMAT_R32G32B32_SFLOAT,
						.offset = offsetof(math::Vertex, tangent)
					},
				})
			.withInputAssembly()
			.withViewportState()
			.withRasterization()
			.withMultisample(storage.msaaSamples)
			.withDepthStencil(true)
			.withColorBlend()
			.withDynamicState()
			.build(storage, storage.mainPipelineLayout, storage.mainPipeline);
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
			if (texture->sampler)     { vkDestroySampler(storage.logicalDevice, texture->sampler, nullptr);         texture->sampler = nullptr; }
			if (texture->imageView)   { vkDestroyImageView(storage.logicalDevice, texture->imageView, nullptr);     texture->imageView = nullptr; }
			if (texture->image)       { vkDestroyImage(storage.logicalDevice, texture->image, nullptr);             texture->image = nullptr; }
			if (texture->imageMemory) { vkFreeMemory(storage.logicalDevice, texture->imageMemory, nullptr);         texture->imageMemory = nullptr; }
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
		std::vector extensions = Services::get<imgui::ImGuiLibrary>()->getRequiredExtensions();

		if (VkInstanceBuilder::validationLayersAreEnabled())
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	void VulkanInitializer::createShadowMapResources(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager)
	{
		constexpr uint32_t shadowMapSize = decltype(storage.shadowMap)::SIZE;
		const VkFormat depthFormat = utils::findDepthFormat(storage);

		// Create shadow map depth image
		storage.shadowMap.image = VkImageBuilder("Shadow Map Image")
			.setWidth(shadowMapSize)
			.setHeight(shadowMapSize)
			.setFormat(depthFormat)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.build(storage);

		storage.shadowMap.imageMemory = VkDeviceMemoryBuilder("Shadow Map Memory")
			.setImage(storage.shadowMap.image)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		const VkImageAspectFlags depthAspect = VK_IMAGE_ASPECT_DEPTH_BIT;
		storage.shadowMap.imageView = VkImageViewBuilder()
			.setImage(storage.shadowMap.image)
			.setViewType(VK_IMAGE_VIEW_TYPE_2D)
			.setFormat(depthFormat)
			.setAspectMask(depthAspect)
			.build("Shadow Map View", storage);

		// Create shadow map sampler with depth comparison
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		samplerInfo.anisotropyEnable = VK_FALSE;
		samplerInfo.compareEnable = VK_TRUE;
		samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 1.0f;

		ASSERT(vkCreateSampler(storage.logicalDevice, &samplerInfo, nullptr, &storage.shadowMap.sampler) == VK_SUCCESS,
			"Failed to create shadow map sampler.");
		utils::setDebugName(storage, storage.shadowMap.sampler, VK_OBJECT_TYPE_SAMPLER, "Shadow Map Sampler");

		// Create shadow render pass (depth-only)
		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = depthFormat;
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

		VkAttachmentReference depthRef{};
		depthRef.attachment = 0;
		depthRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 0;
		subpass.pColorAttachments = nullptr;
		subpass.pDepthStencilAttachment = &depthRef;

		std::array<VkSubpassDependency, 2> dependencies{};
		dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[0].dstSubpass = 0;
		dependencies[0].srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[0].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependencies[0].srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[0].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		dependencies[1].srcSubpass = 0;
		dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
		dependencies[1].srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		dependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &depthAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassInfo.pDependencies = dependencies.data();

		ASSERT(vkCreateRenderPass(storage.logicalDevice, &renderPassInfo, nullptr, &storage.shadowRenderPass) == VK_SUCCESS,
			"Failed to create shadow render pass.");
		utils::setDebugName(storage, storage.shadowRenderPass, VK_OBJECT_TYPE_RENDER_PASS, "Shadow Map Render Pass");

		// Create shadow framebuffer
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = storage.shadowRenderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &storage.shadowMap.imageView;
		framebufferInfo.width = shadowMapSize;
		framebufferInfo.height = shadowMapSize;
		framebufferInfo.layers = 1;

		ASSERT(vkCreateFramebuffer(storage.logicalDevice, &framebufferInfo, nullptr, &storage.shadowMap.framebuffer) == VK_SUCCESS,
			"Failed to create shadow map framebuffer.");
		utils::setDebugName(storage, storage.shadowMap.framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "Shadow Map Framebuffer");

		// Create shadow pipeline
		using DescriptorType = VulkanDescriptorManager::DescriptorType;
		VkPipelineBuilder("Shadow Pipeline")
			.setRenderPass(storage.shadowRenderPass)
			.useLayouts(
				{
					descriptorManager.getLayout(DescriptorType::GLOBAL),
					descriptorManager.getLayout(DescriptorType::INSTANCE),
				})
			.addStage(storage, VK_SHADER_STAGE_VERTEX_BIT, "bin/shaders/shadow.vert.spv")
			.withVertexInput(
				{
					{
						.binding = 0,
						.stride = sizeof(math::Vertex),
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
					}
				},
				{
					{
						.location = 0,
						.binding = 0,
						.format = VK_FORMAT_R32G32B32_SFLOAT,
						.offset = offsetof(math::Vertex, position)
					}
				})
			.withInputAssembly()
			.withViewportState()
			.withRasterization(VK_CULL_MODE_FRONT_BIT, true, 1.25f, 1.75f)
			.withMultisample(VK_SAMPLE_COUNT_1_BIT)
			.withDepthStencil(true)
			.withNoColorBlend()
			.withDynamicState()
			.build(storage, storage.shadowPipelineLayout, storage.shadowPipeline);
	}

	void VulkanInitializer::createSSAOResources(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager)
	{
		using DescriptorType = VulkanDescriptorManager::DescriptorType;

		const uint32_t screenWidth = storage.swapChainDetails.swapChainExtent.width;
		const uint32_t screenHeight = storage.swapChainDetails.swapChainExtent.height;
		const VkFormat depthFormat = utils::findDepthFormat(storage);

		// ===================================================================
		// 1. Depth Pre-pass Resources
		// ===================================================================

		storage.depthPrePass.image = VkImageBuilder("Depth Pre-Pass Image")
			.setWidth(screenWidth)
			.setHeight(screenHeight)
			.setFormat(depthFormat)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.build(storage);

		storage.depthPrePass.imageMemory = VkDeviceMemoryBuilder("Depth Pre-Pass Memory")
			.setImage(storage.depthPrePass.image)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		storage.depthPrePass.imageView = VkImageViewBuilder()
			.setImage(storage.depthPrePass.image)
			.setViewType(VK_IMAGE_VIEW_TYPE_2D)
			.setFormat(depthFormat)
			.setAspectMask(VK_IMAGE_ASPECT_DEPTH_BIT)
			.build("Depth Pre-Pass View", storage);

		storage.depthPrePass.sampler = VkSamplerBuilder("Depth Pre-Pass Sampler")
			.setSamplerMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
			.setBorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
			.build(storage);

		// Depth pre-pass render pass
		{
			const VkAttachmentDescription depthAttachment =
			{
				.flags = 0,
				.format = depthFormat,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
			};

			const VkAttachmentReference depthAttachmentRef =
			{
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			};

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 0;
			subpass.pColorAttachments = nullptr;
			subpass.pDepthStencilAttachment = &depthAttachmentRef;

			const std::array<VkSubpassDependency, 2> dependencies =
			{{
				{
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					.srcAccessMask = VK_ACCESS_NONE,
					.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
				},
				{
					.srcSubpass = 0,
					.dstSubpass = VK_SUBPASS_EXTERNAL,
					.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
				}
			}};

			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &depthAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassInfo.pDependencies = dependencies.data();

			ASSERT(vkCreateRenderPass(storage.logicalDevice, &renderPassInfo, nullptr, &storage.depthPrePassRenderPass) == VK_SUCCESS,
				"Failed to create depth pre-pass render pass.");
			utils::setDebugName(storage, storage.depthPrePassRenderPass, VK_OBJECT_TYPE_RENDER_PASS, "Depth Pre-Pass Render Pass");
		}

		// Depth pre-pass framebuffer
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = storage.depthPrePassRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &storage.depthPrePass.imageView;
			framebufferInfo.width = screenWidth;
			framebufferInfo.height = screenHeight;
			framebufferInfo.layers = 1;

			ASSERT(vkCreateFramebuffer(storage.logicalDevice, &framebufferInfo, nullptr, &storage.depthPrePass.framebuffer) == VK_SUCCESS,
				"Failed to create depth pre-pass framebuffer.");
			utils::setDebugName(storage, storage.depthPrePass.framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "Depth Pre-Pass Framebuffer");
		}

		// Depth pre-pass pipeline
		VkPipelineBuilder("Depth Pre-Pass Pipeline")
			.setRenderPass(storage.depthPrePassRenderPass)
			.useLayouts({
				descriptorManager.getLayout(DescriptorType::GLOBAL),
				descriptorManager.getLayout(DescriptorType::INSTANCE)
			})
			.addStage(storage, VK_SHADER_STAGE_VERTEX_BIT, "bin/shaders/depthprepass.vert.spv")
			.withVertexInput(
				{
					{
						.binding = 0,
						.stride = sizeof(math::Vertex),
						.inputRate = VK_VERTEX_INPUT_RATE_VERTEX
					}
				},
				{
					{
						.location = 0,
						.binding = 0,
						.format = VK_FORMAT_R32G32B32_SFLOAT,
						.offset = offsetof(math::Vertex, position)
					}
				})
			.withInputAssembly()
			.withViewportState()
			.withRasterization()
			.withMultisample(VK_SAMPLE_COUNT_1_BIT)
			.withDepthStencil(true)
			.withNoColorBlend()
			.withDynamicState()
			.build(storage, storage.depthPrePassPipelineLayout, storage.depthPrePassPipeline);

		// ===================================================================
		// 2. SSAO Pass Resources
		// ===================================================================

		// SSAO render pass
		{
			const VkAttachmentDescription colorAttachment =
			{
				.flags = 0,
				.format = VK_FORMAT_R16_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			const VkAttachmentReference colorAttachmentRef =
			{
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			const std::array<VkSubpassDependency, 2> dependencies =
			{{
				{
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
				},
				{
					.srcSubpass = 0,
					.dstSubpass = VK_SUBPASS_EXTERNAL,
					.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
				}
			}};

			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassInfo.pDependencies = dependencies.data();

			ASSERT(vkCreateRenderPass(storage.logicalDevice, &renderPassInfo, nullptr, &storage.ssaoRenderPass) == VK_SUCCESS,
				"Failed to create SSAO render pass.");
			utils::setDebugName(storage, storage.ssaoRenderPass, VK_OBJECT_TYPE_RENDER_PASS, "SSAO Render Pass");
		}

		// SSAO image
		storage.ssaoBuffer.image = VkImageBuilder("SSAO Image")
			.setWidth(screenWidth)
			.setHeight(screenHeight)
			.setFormat(VK_FORMAT_R16_SFLOAT)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.build(storage);

		storage.ssaoBuffer.imageMemory = VkDeviceMemoryBuilder("SSAO Image Memory")
			.setImage(storage.ssaoBuffer.image)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		storage.ssaoBuffer.imageView = VkImageViewBuilder()
			.setImage(storage.ssaoBuffer.image)
			.setViewType(VK_IMAGE_VIEW_TYPE_2D)
			.setFormat(VK_FORMAT_R16_SFLOAT)
			.setAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.build("SSAO Image View", storage);

		// SSAO framebuffer
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = storage.ssaoRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &storage.ssaoBuffer.imageView;
			framebufferInfo.width = screenWidth;
			framebufferInfo.height = screenHeight;
			framebufferInfo.layers = 1;

			ASSERT(vkCreateFramebuffer(storage.logicalDevice, &framebufferInfo, nullptr, &storage.ssaoBuffer.framebuffer) == VK_SUCCESS,
				"Failed to create SSAO framebuffer.");
			utils::setDebugName(storage, storage.ssaoBuffer.framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "SSAO Framebuffer");
		}

		// SSAO descriptor set layout
		storage.ssaoDescriptorSetLayout = VkDescriptorSetLayoutBuilder("SSAO Descriptor Set Layout")
			.withBindings({
				{ "Binding 0: Depth Pre-Pass Texture", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
			})
			.build(storage);

		// SSAO descriptor set
		{
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = storage.descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &storage.ssaoDescriptorSetLayout;

			ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &allocInfo, &storage.ssaoDescriptorSet) == VK_SUCCESS,
				"Failed to allocate SSAO descriptor set.");

			const VkDescriptorImageInfo depthImageInfo =
			{
				.sampler = storage.depthPrePass.sampler,
				.imageView = storage.depthPrePass.imageView,
				.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			};

			const VkWriteDescriptorSet write =
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = storage.ssaoDescriptorSet,
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &depthImageInfo,
				.pBufferInfo = nullptr,
				.pTexelBufferView = nullptr
			};

			vkUpdateDescriptorSets(storage.logicalDevice, 1, &write, 0, nullptr);
		}

		// SSAO pipeline
		VkPipelineBuilder("SSAO Pipeline")
			.setRenderPass(storage.ssaoRenderPass)
			.useLayouts({
				descriptorManager.getLayout(DescriptorType::GLOBAL),
				storage.ssaoDescriptorSetLayout
			})
			.addStage(storage, VK_SHADER_STAGE_VERTEX_BIT, "bin/shaders/ssao.vert.spv")
			.addStage(storage, VK_SHADER_STAGE_FRAGMENT_BIT, "bin/shaders/ssao.frag.spv")
			.withVertexInput({}, {})
			.withInputAssembly()
			.withViewportState()
			.withRasterization(VK_CULL_MODE_NONE, false, 0.0f, 0.0f)
			.withMultisample(VK_SAMPLE_COUNT_1_BIT)
			.withColorBlend()
			.withDynamicState()
			.build(storage, storage.ssaoPipelineLayout, storage.ssaoPipeline);

		// ===================================================================
		// 3. SSAO Blur Pass Resources
		// ===================================================================

		// SSAO blur render pass
		{
			const VkAttachmentDescription colorAttachment =
			{
				.flags = 0,
				.format = VK_FORMAT_R16_SFLOAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			};

			const VkAttachmentReference colorAttachmentRef =
			{
				.attachment = 0,
				.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			};

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			const std::array<VkSubpassDependency, 2> dependencies =
			{{
				{
					.srcSubpass = VK_SUBPASS_EXTERNAL,
					.dstSubpass = 0,
					.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
					.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
				},
				{
					.srcSubpass = 0,
					.dstSubpass = VK_SUBPASS_EXTERNAL,
					.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
					.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
					.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
					.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
					.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT,
				}
			}};

			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassInfo.pDependencies = dependencies.data();

			ASSERT(vkCreateRenderPass(storage.logicalDevice, &renderPassInfo, nullptr, &storage.ssaoBlurRenderPass) == VK_SUCCESS,
				"Failed to create SSAO blur render pass.");
			utils::setDebugName(storage, storage.ssaoBlurRenderPass, VK_OBJECT_TYPE_RENDER_PASS, "SSAO Blur Render Pass");
		}

		// SSAO blur image
		storage.ssaoBlurBuffer.image = VkImageBuilder("SSAO Blur Image")
			.setWidth(screenWidth)
			.setHeight(screenHeight)
			.setFormat(VK_FORMAT_R16_SFLOAT)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.build(storage);

		storage.ssaoBlurBuffer.imageMemory = VkDeviceMemoryBuilder("SSAO Blur Image Memory")
			.setImage(storage.ssaoBlurBuffer.image)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		storage.ssaoBlurBuffer.imageView = VkImageViewBuilder()
			.setImage(storage.ssaoBlurBuffer.image)
			.setViewType(VK_IMAGE_VIEW_TYPE_2D)
			.setFormat(VK_FORMAT_R16_SFLOAT)
			.setAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.build("SSAO Blur Image View", storage);

		storage.ssaoBlurBuffer.sampler = VkSamplerBuilder("SSAO Blur Sampler")
			.setSamplerMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
			.setBorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
			.build(storage);

		// SSAO blur framebuffer
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = storage.ssaoBlurRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &storage.ssaoBlurBuffer.imageView;
			framebufferInfo.width = screenWidth;
			framebufferInfo.height = screenHeight;
			framebufferInfo.layers = 1;

			ASSERT(vkCreateFramebuffer(storage.logicalDevice, &framebufferInfo, nullptr, &storage.ssaoBlurBuffer.framebuffer) == VK_SUCCESS,
				"Failed to create SSAO blur framebuffer.");
			utils::setDebugName(storage, storage.ssaoBlurBuffer.framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "SSAO Blur Framebuffer");
		}

		// SSAO blur descriptor set layout
		storage.ssaoBlurDescriptorSetLayout = VkDescriptorSetLayoutBuilder("SSAO Blur Descriptor Set Layout")
			.withBindings({
				{ "Binding 0: SSAO Texture", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
				{ "Binding 1: Depth Map", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT }
			})
			.build(storage);

		// SSAO blur descriptor set
		{
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = storage.descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &storage.ssaoBlurDescriptorSetLayout;

			ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &allocInfo, &storage.ssaoBlurDescriptorSet) == VK_SUCCESS,
				"Failed to allocate SSAO blur descriptor set.");

			const VkDescriptorImageInfo ssaoImageInfo =
			{
				.sampler = storage.depthPrePass.sampler,
				.imageView = storage.ssaoBuffer.imageView,
				.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			};

			const VkDescriptorImageInfo depthImageInfo =
			{
				.sampler = storage.depthPrePass.sampler,
				.imageView = storage.depthPrePass.imageView,
				.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			};

			const std::array<VkWriteDescriptorSet, 2> writes =
			{
				VkWriteDescriptorSet
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = storage.ssaoBlurDescriptorSet,
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &ssaoImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				},
				VkWriteDescriptorSet
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = storage.ssaoBlurDescriptorSet,
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &depthImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				}
			};

			vkUpdateDescriptorSets(storage.logicalDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
		}

		// SSAO blur pipeline
		VkPipelineBuilder("SSAO Blur Pipeline")
			.setRenderPass(storage.ssaoBlurRenderPass)
			.useLayouts({ storage.ssaoBlurDescriptorSetLayout })
			.addStage(storage, VK_SHADER_STAGE_VERTEX_BIT, "bin/shaders/ssao.vert.spv")
			.addStage(storage, VK_SHADER_STAGE_FRAGMENT_BIT, "bin/shaders/ssao_blur.frag.spv")
			.withVertexInput({}, {})
			.withInputAssembly()
			.withViewportState()
			.withRasterization(VK_CULL_MODE_NONE, false, 0.0f, 0.0f)
			.withMultisample(VK_SAMPLE_COUNT_1_BIT)
			.withColorBlend()
			.withDynamicState()
			.build(storage, storage.ssaoBlurPipelineLayout, storage.ssaoBlurPipeline);
	}

	void VulkanInitializer::cleanupSSAOResources(const VulkanStorage& storage)
	{
		vkDestroyPipeline(storage.logicalDevice, storage.ssaoBlurPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.ssaoBlurPipelineLayout, nullptr);
		vkDestroyRenderPass(storage.logicalDevice, storage.ssaoBlurRenderPass, nullptr);
		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.ssaoBlurDescriptorSetLayout, nullptr);
		vkDestroyFramebuffer(storage.logicalDevice, storage.ssaoBlurBuffer.framebuffer, nullptr);
		vkDestroySampler(storage.logicalDevice, storage.ssaoBlurBuffer.sampler, nullptr);
		vkDestroyImageView(storage.logicalDevice, storage.ssaoBlurBuffer.imageView, nullptr);
		vkDestroyImage(storage.logicalDevice, storage.ssaoBlurBuffer.image, nullptr);
		vkFreeMemory(storage.logicalDevice, storage.ssaoBlurBuffer.imageMemory, nullptr);

		vkDestroyPipeline(storage.logicalDevice, storage.ssaoPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.ssaoPipelineLayout, nullptr);
		vkDestroyRenderPass(storage.logicalDevice, storage.ssaoRenderPass, nullptr);
		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.ssaoDescriptorSetLayout, nullptr);
		vkDestroyFramebuffer(storage.logicalDevice, storage.ssaoBuffer.framebuffer, nullptr);
		vkDestroyImageView(storage.logicalDevice, storage.ssaoBuffer.imageView, nullptr);
		vkDestroyImage(storage.logicalDevice, storage.ssaoBuffer.image, nullptr);
		vkFreeMemory(storage.logicalDevice, storage.ssaoBuffer.imageMemory, nullptr);

		vkDestroyPipeline(storage.logicalDevice, storage.depthPrePassPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.depthPrePassPipelineLayout, nullptr);
		vkDestroyRenderPass(storage.logicalDevice, storage.depthPrePassRenderPass, nullptr);
		vkDestroyFramebuffer(storage.logicalDevice, storage.depthPrePass.framebuffer, nullptr);
		vkDestroySampler(storage.logicalDevice, storage.depthPrePass.sampler, nullptr);
		vkDestroyImageView(storage.logicalDevice, storage.depthPrePass.imageView, nullptr);
		vkDestroyImage(storage.logicalDevice, storage.depthPrePass.image, nullptr);
		vkFreeMemory(storage.logicalDevice, storage.depthPrePass.imageMemory, nullptr);
	}

}
