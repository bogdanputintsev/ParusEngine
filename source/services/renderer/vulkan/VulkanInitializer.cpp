#include "VulkanInitializer.h"

#include "builder/VkDebugUtilsBuilder.h"
#include "builder/VkDescriptorPoolBuilder.h"
#include "builder/VkDescriptorSetLayoutBuilder.h"
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
#include "builder/VulkanTexture2dBuilder.h"
#include "engine/utils/math/UniformBufferObjects.h"
#include "services/Services.h"
#include "services/config/Configs.h"
#include "services/graphics/imgui/ImGuiLibrary.h"
#include "services/world/World.h"
#include "utils/VulkanUtils.h"
#include "VulkanRenderer.h"
#include "builder/VkCommandBufferBuilder.h"


namespace parus::vulkan
{
	
	void VulkanInitializer::initialize(VulkanStorage& storage)
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
		createDescriptorSetLayout(storage);
		createDescriptorPool(storage);
		createSkyPipeline(storage);
		createGraphicsPipeline(storage);
		createColorResources(storage);
		createDepthResources(storage);
		createFramebuffers(storage);
		createUniformBuffer(storage);

		VkSyncObjectsBuilder().build(storage);
		
		storage.commandBuffers = VkCommandBufferBuilder()
			.setCommandPool(utils::getCommandPool(storage))
			.setCount(VulkanStorage::MAX_FRAMES_IN_FLIGHT)
			.build("Main Command Buffer", storage);
	}

	void VulkanInitializer::cleanup(VulkanStorage& storage)
	{
		cleanupSwapChain(storage);

		vkDestroyPipeline(storage.logicalDevice, storage.mainPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.mainPipelineLayout, nullptr);
		vkDestroyPipeline(storage.logicalDevice, storage.skyPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.skyPipelineLayout, nullptr);
		vkDestroyRenderPass(storage.logicalDevice, storage.renderPass, nullptr);

		for (size_t i = 0; i < VulkanStorage::MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(storage.logicalDevice, storage.globalUboBuffer.frameBuffers[i], nullptr);
			vkDestroyBuffer(storage.logicalDevice, storage.instanceUboBuffer.frameBuffers[i], nullptr);
			vkDestroyBuffer(storage.logicalDevice, storage.directionalLightUboBuffer.frameBuffers[i], nullptr);
			vkFreeMemory(storage.logicalDevice, storage.globalUboBuffer.memory[i], nullptr);
			vkFreeMemory(storage.logicalDevice, storage.instanceUboBuffer.memory[i], nullptr);
			vkFreeMemory(storage.logicalDevice, storage.directionalLightUboBuffer.memory[i], nullptr);
		}

		vkDestroyDescriptorPool(storage.logicalDevice, storage.descriptorPool, nullptr);
		cleanupTextures(storage);

		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.globalDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.instanceDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.materialDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.lightsDescriptorSetLayout, nullptr);

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
		Services::get<imgui::ImGuiLibrary>()->handleMinimization();
		Services::get<VulkanRenderer>()->deviceWaitIdle();

		cleanupSwapChain(storage);

		createSwapChain(storage);
		createColorResources(storage);
		createDepthResources(storage);
		createFramebuffers(storage);
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

	void VulkanInitializer::createDescriptorSetLayout(VulkanStorage& storage)
	{
		storage.globalDescriptorSetLayout = VkDescriptorSetLayoutBuilder("Descriptor Set 0 - Global UBO")
			.withBindings({
				{
					.bindingName = "Binding 0: Global UBO",
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
				}})
			.build(storage);

		storage.instanceDescriptorSetLayout = VkDescriptorSetLayoutBuilder("Descriptor Set 1 - Instance UBO")
			.withBindings({
				{
					.bindingName = "Binding 0: Instance UBO",
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.stageFlags = VK_SHADER_STAGE_VERTEX_BIT
				}})
			.build(storage);

		storage.materialDescriptorSetLayout = VkDescriptorSetLayoutBuilder("Descriptor Set 2 - Material")
			.withBindings({
				{
					.bindingName = "Binding 0: Albedo",
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				},
				{
					.bindingName = "Binding 1: Normal",
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				},
				{
					.bindingName = "Binding 2: Metallic",
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				},
				{
					.bindingName = "Binding 3: Roughness",
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				},
				{
					.bindingName = "Binding 4: Ambient Occlusion",
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				}})
			.build(storage);

		storage.lightsDescriptorSetLayout = VkDescriptorSetLayoutBuilder("Descriptor Set 3 - Lights")
			.withBindings({
				{
					.bindingName = "Binding 0: Light UBO",
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				},
				{
					.bindingName = "Binding 1: Cube map",
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
				}})
			.build(storage);
	}

	void VulkanInitializer::createDescriptorPool(VulkanStorage& storage)
	{
		constexpr size_t MAX_NUMBER_OF_MESHES = 100;
		constexpr int IMAGE_SAMPLER_POOL_SIZE = 1000;

		VkDescriptorPoolBuilder()
			.setMaxSets(VulkanStorage::MAX_FRAMES_IN_FLIGHT * MAX_NUMBER_OF_MESHES * NUMBER_OF_TEXTURE_TYPES + IMAGE_SAMPLER_POOL_SIZE + 1)
			.setPoolSizes({{
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = VulkanStorage::MAX_FRAMES_IN_FLIGHT
				},
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = VulkanStorage::MAX_FRAMES_IN_FLIGHT * MAX_NUMBER_OF_MESHES * 2
				},
				{
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = MAX_NUMBER_OF_MESHES * NUMBER_OF_TEXTURE_TYPES + IMAGE_SAMPLER_POOL_SIZE
				},
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = VulkanStorage::MAX_FRAMES_IN_FLIGHT
				},
			}})
			.build(storage);
	}

	void VulkanInitializer::createSkyPipeline(VulkanStorage& storage)
	{
		VkPipelineBuilder("Sky Pipeline Layout")
			.useLayouts({storage.globalDescriptorSetLayout})
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

	void VulkanInitializer::createGraphicsPipeline(VulkanStorage& storage)
	{
		VkPipelineBuilder("Main Pipeline Layout")
			.useLayouts(
				{
					storage.globalDescriptorSetLayout,
					storage.instanceDescriptorSetLayout,
					storage.materialDescriptorSetLayout,
					storage.lightsDescriptorSetLayout
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

}
