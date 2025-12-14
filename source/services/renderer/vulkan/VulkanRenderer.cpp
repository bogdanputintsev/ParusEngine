#include "VulkanRenderer.h"

#include <array>
#include <cassert>
#include <set>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

#include "builder/VkDebugUtilsBuilder.h"
#include "builder/VkDescriptorPoolBuilder.h"
#include "builder/VkDescriptorSetLayoutBuilder.h"
#include "builder/VkDeviceBuilder.h"
#include "builder/VkDeviceMemoryBuilder.h"
#include "builder/VkImageBuilder.h"
#include "builder/VkImageViewBuilder.h"
#include "builder/VkInstanceBuilder.h"
#include "builder/VkPipelineBuilder.h"
#include "builder/VkQueuesBuilder.h"
#include "builder/VkRenderPassBuilder.h"
#include "builder/VkSamplerBuilder.h"
#include "builder/VkSurfaceBuilder.h"
#include "builder/VkSwapChainBuilder.h"
#include "builder/VulkanTexture2dBuilder.h"
#include "engine/input/Input.h"
#include "services/platform/Platform.h"
#include "engine/Event.h"
#include "engine/utils/Utils.h"
#include "services/graphics/imgui/ImGuiLibrary.h"
#include "engine/utils/math/UniformBufferObjects.h"
#include "material/Material.h"
#include "services/Services.h"
#include "services/config/Configs.h"
#include "services/threading/ThreadPool.h"
#include "services/world/World.h"
#include "utils/VulkanUtils.h"


namespace parus::vulkan
{
	void VulkanRenderer::init()
	{
		registerEvents();
		
		createInstance();
		createDebugManager();
		createSurface();
		createDevices();
		createQueues();
		createSwapChain();
		createRenderPass();
		createDescriptorSetLayout();
		createDescriptorPool();
		createSkyPipeline();
		createGraphicsPipeline();
		
		createColorResources();
		createDepthResources();
		createFramebuffers();

		createUniformBuffer();

		directionalLight = 
			{
				.light = {
					.color = math::Vector3(1.0f, 0.65f, 0.8f).trivial(),
					.direction = math::Vector3(66.0f, 70.0f, 429.0f).trivial()
				},
				.descriptorSets = {}
			};

		createCubemapTexture();

		// Load sky mesh
		importMesh("bin/assets/skybox/dynamic_skybox.obj", MeshType::SKY);
		
		RUN_ASYNC(importMesh("bin/assets/terrain/floor.obj"););
		RUN_ASYNC(importMesh("bin/assets/indoor/indoor.obj"););
		RUN_ASYNC(importMesh("bin/assets/indoor/threshold.obj"););
		RUN_ASYNC(importMesh("bin/assets/indoor/torch.obj"););

		createCommandBuffer();
		createSyncObjects();

		isRunning = true;
	}

	void VulkanRenderer::registerEvents()
	{
		REGISTER_EVENT(EventType::EVENT_WINDOW_RESIZED, [&](const int newWidth, const int newHeight)
		{
			// FIXME: #19 Crash when resizing or minimizing window in default scene
			LOG_INFO("Vulkan initiated window resize. New dimensions: " + std::to_string(newWidth) + " " + std::to_string(newHeight));
			onResize();
		});

		REGISTER_EVENT(EventType::EVENT_KEY_PRESSED, [&](const KeyButton key)
		{
			if (key == KeyButton::KEY_Z)
			{
				isDrawDebugEnabled = !isDrawDebugEnabled;
			}
		});

		REGISTER_EVENT(EventType::EVENT_APPLICATION_QUIT, [&]([[maybe_unused]]const int exitCode)
		{
			isRunning = false;
		});
	}

	void VulkanRenderer::clean()
	{
		cleanupSwapChain();

		vkDestroyPipeline(storage.logicalDevice, storage.mainPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.mainPipelineLayout, nullptr);
		vkDestroyPipeline(storage.logicalDevice, storage.skyPipeline, nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, storage.skyPipelineLayout, nullptr);
		vkDestroyRenderPass(storage.logicalDevice, storage.renderPass, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroyBuffer(storage.logicalDevice, globalUboBuffer.frameBuffers[i], nullptr);
			vkDestroyBuffer(storage.logicalDevice, instanceUboBuffer.frameBuffers[i], nullptr);
			vkDestroyBuffer(storage.logicalDevice, directionalLightUboBuffer.frameBuffers[i], nullptr);
			vkFreeMemory(storage.logicalDevice, globalUboBuffer.memory[i], nullptr);
			vkFreeMemory(storage.logicalDevice, instanceUboBuffer.memory[i], nullptr);
			vkFreeMemory(storage.logicalDevice, directionalLightUboBuffer.memory[i], nullptr);
		}

		vkDestroyDescriptorPool(storage.logicalDevice, storage.descriptorPool, nullptr);
		
		// Clean textures
		const std::vector<std::shared_ptr<VulkanTexture2d>> allTextures = Services::get<World>()->getStorage()->getAllTextures();
		for (const auto& texture : allTextures)
		{
			if (texture->sampler)
			{
				vkDestroySampler(storage.logicalDevice, texture->sampler, nullptr);
				texture->sampler = nullptr;
			}
			if (texture->imageView)
			{
				vkDestroyImageView(storage.logicalDevice, texture->imageView, nullptr);
				texture->imageView = nullptr;
			}
			if (texture->image)
			{
				vkDestroyImage(storage.logicalDevice, texture->image, nullptr);
				texture->image = nullptr;
			}
			if (texture->imageMemory)
			{
				vkFreeMemory(storage.logicalDevice, texture->imageMemory, nullptr);
				texture->imageMemory = nullptr;
			}
		}

		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.globalDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.instanceDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.materialDescriptorSetLayout, nullptr);
		vkDestroyDescriptorSetLayout(storage.logicalDevice, storage.lightsDescriptorSetLayout, nullptr);

		vkDestroyBuffer(storage.logicalDevice, globalBuffers.indexBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, globalBuffers.indexBufferMemory, nullptr);
		
		vkDestroyBuffer(storage.logicalDevice, globalBuffers.vertexBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, globalBuffers.vertexBufferMemory, nullptr);

		vkDestroyBuffer(storage.logicalDevice, globalBuffers.skyIndexBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, globalBuffers.skyIndexBufferMemory, nullptr);

		vkDestroyBuffer(storage.logicalDevice, globalBuffers.skyVertexBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, globalBuffers.skyVertexBufferMemory, nullptr);
		
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(storage.logicalDevice, renderFinishedSemaphores[i], nullptr);
			vkDestroySemaphore(storage.logicalDevice, imageAvailableSemaphores[i], nullptr);
			vkDestroyFence(storage.logicalDevice, inFlightFences[i], nullptr);
		}

		for (const auto& [_, commandPool]: threadCommandPools)
		{
			vkDestroyCommandPool(storage.logicalDevice, commandPool, nullptr);
		}

		vkDestroyDevice(storage.logicalDevice, nullptr);

		if (validationLayersAreEnabled())
		{
			destroyDebugUtilsMessengerExt(storage.debugMessenger, nullptr);
		}

		vkDestroySurfaceKHR(storage.instance, storage.surface, nullptr);
		vkDestroyInstance(storage.instance, nullptr);
	}

	void VulkanRenderer::drawFrame()
	{
		if (!isRunning)
		{
			return;
		}
		
		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		cleanupFrameResources();
		
		vkWaitForFences(storage.logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		const std::optional<uint32_t> imageIndex = acquireNextImage();
		if (!imageIndex.has_value())
		{
			return;
		}

		updateUniformBuffer(currentFrame);
		processLoadedMeshes();
		vkResetFences(storage.logicalDevice, 1, &inFlightFences[currentFrame]);

		const auto commandBuffer = getCommandBuffer(currentFrame);

		resetCommandBuffer(currentFrame);
		recordCommandBuffer(commandBuffer, imageIndex.value());

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		const VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
		constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		const VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		ASSERT(utils::threadSafeQueueSubmit(storage, &submitInfo, inFlightFences[currentFrame]) == VK_SUCCESS, "failed to submit draw command buffer.");

		// Submit result back to swap chain to have it eventually show up on the screen. 
		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		const VkSwapchainKHR swapChains[] = { storage.swapChain };
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex.value();
		presentInfo.pResults = nullptr;
		
		const VkResult result = utils::threadSafePresent(storage, &presentInfo);

		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			framebufferResized = false;
			recreateSwapChain();
		}
		else
		{
			ASSERT(result == VK_SUCCESS, "failed to present swap chain image.");
		}

	}

	void VulkanRenderer::deviceWaitIdle()
	{
		vkDeviceWaitIdle(storage.logicalDevice);
	}
	
	void VulkanRenderer::importMesh(const std::string& meshPath, const MeshType meshType)
	{
		Mesh newMesh = importMeshFromFile(meshPath);
		newMesh.meshType = meshType;

		std::scoped_lock lock(importModelMutex);
		modelQueue.emplace(meshPath, std::make_shared<Mesh>(newMesh));
	}

	void VulkanRenderer::processLoadedMeshes()
	{
		std::unique_lock lock(importModelMutex);
		if (modelQueue.empty())
		{
			return;	
		}
		
		while (!modelQueue.empty())
		{
			auto [meshPath, newMesh] = modelQueue.front();
			modelQueue.pop();
			lock.unlock();
			
			Services::get<World>()->getStorage()->addNewMesh(meshPath, newMesh);

			meshInstances.push_back({
				.mesh = newMesh,
				.transform = math::Matrix4x4::identity(),
				.instanceDescriptorSets = {}
			});
		}

		// ==== [ MAIN SCENE BUFFERS ] ====
		std::vector<math::Vertex> allVertices;
		std::vector<uint32_t> allIndices;

		for (const auto& mesh : Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::STATIC_MESH))
		{
			for (auto& meshPart : mesh->meshParts)
			{
				meshPart.vertexOffset = allVertices.size();
				meshPart.indexOffset = allIndices.size();

				allVertices.insert(allVertices.end(), meshPart.vertices.begin(), meshPart.vertices.end());
				allIndices.insert(allIndices.end(), meshPart.indices.begin(), meshPart.indices.end());
			}
		}

		if (!allVertices.empty())
		{
			createVertexBuffer(allVertices);
		}
		if (!allIndices.empty())
		{
			createIndexBuffer(allIndices);
		}
		
		globalBuffers.totalVertices = allVertices.size();
		globalBuffers.totalIndices = allIndices.size();

		// ==== [ SKY BUFFERS ] ====
		std::vector<math::Vertex> allSkyVertices;
		std::vector<uint32_t> allSkyIndices;

		DEBUG_ASSERT(Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::SKY).size() == 1,
			"There must be always one and only one sky mesh.");
		
		for (const auto& mesh : Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::SKY))
		{
			for (auto& meshPart : mesh->meshParts)
			{
				meshPart.vertexOffset = allSkyVertices.size();
				meshPart.indexOffset = allSkyIndices.size();

				allSkyVertices.insert(allSkyVertices.end(), meshPart.vertices.begin(), meshPart.vertices.end());
				allSkyIndices.insert(allSkyIndices.end(), meshPart.indices.begin(), meshPart.indices.end());
			}
		}

		if (!allSkyVertices.empty())
		{
			createSkyVertexBuffer(allSkyVertices);
		}
		if (!allSkyVertices.empty())
		{
			createSkyIndexBuffer(allSkyIndices);
		}
		
		globalBuffers.totalSkyVertices = allSkyVertices.size();
		globalBuffers.totalSkyIndices = allSkyIndices.size();
		
		for (auto& mesh : Services::get<World>()->getStorage()->getAllMeshes())
		{
			createMeshDescriptorSets(mesh);
		}

		createLightsDescriptorSets();

	}

	void VulkanRenderer::createInstance()
	{
		checkValidationLayerSupport();
		checkIfAllRequiredExtensionsAreSupported();
		
        const std::string applicationName = Services::get<Configs>()->get("Engine", "applicationName");
        const int versionMajor = Services::get<Configs>()->getAsInt("Engine", "versionMajor").value_or(0);
        const int versionMinor = Services::get<Configs>()->getAsInt("Engine", "versionMinor").value_or(0);
        const int versionPatch = Services::get<Configs>()->getAsInt("Engine", "versionPatch").value_or(0);
		
		VkInstanceBuilder()
			.setApplicationName(applicationName)
			.setVersion(versionMajor, versionMinor, versionPatch)
			.setDebugCallback(debugCallback)
			.setRequiredExtensions(getRequiredExtensions())
			.setValidationLayers(getValidationLayers())
			.build(storage);
	}

	void VulkanRenderer::checkValidationLayerSupport()
	{
		if (!validationLayersAreEnabled())
		{
			return;
		}

		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		bool result = true;

		for (const char* layerName : getValidationLayers())
		{
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers)
			{
				if (strcmp(layerName, layerProperties.layerName) == 0)
				{
					layerFound = true;
					break;
				}
			}

			if (!layerFound)
			{
				result = false;
				break;
			}
		}

		ASSERT(result, "validation layers requested, but not available.");

	}

	bool VulkanRenderer::validationLayersAreEnabled()
	{
#ifdef IN_DEBUG_MODE
		return true;
#else
		return false;
#endif
	}

	std::vector<const char*> VulkanRenderer::getValidationLayers()
	{
		return {
			"VK_LAYER_KHRONOS_validation"
		};
	}

	void VulkanRenderer::checkIfAllRequiredExtensionsAreSupported()
	{
		const std::vector<const char*> requiredExtensions = getRequiredExtensions();
		const std::unordered_set<std::string> requiredExtensionsSet{ requiredExtensions.begin(), requiredExtensions.end() };

		uint32_t matches = 0;

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		LOG_DEBUG("List of all available extensions:");

		for (const auto& [extensionName, specVersion] : extensions)
		{
			LOG_DEBUG("\t" + std::string(extensionName));

			if (requiredExtensionsSet.contains(std::string(extensionName)))
			{
				++matches;
			}
		}

		ASSERT(matches == requiredExtensions.size(), "All required Vulkan extensions muse be supported.");
	}

	void VulkanRenderer::destroyDebugUtilsMessengerExt(VkDebugUtilsMessengerEXT debugMessengerToDestroy, const VkAllocationCallbacks* pAllocator) const
	{
		const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(storage.instance, "vkDestroyDebugUtilsMessengerEXT"));
		if (func != nullptr)
		{
			func(storage.instance, debugMessengerToDestroy, pAllocator);
		}
	}

	std::vector<const char*> VulkanRenderer::getRequiredExtensions()
	{
		std::vector extensions = Services::get<imgui::ImGuiLibrary>()->getRequiredExtensions();

		if (validationLayersAreEnabled())
		{
			extensions.emplace_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		}

		return extensions;
	}

	VkBool32 VulkanRenderer::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		[[maybe_unused]] VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		[[maybe_unused]] void* pUserData)
	{
		LOG(getLogType(messageSeverity), pCallbackData->pMessage);
		return VK_FALSE;
	}

	void VulkanRenderer::populate(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	LogType VulkanRenderer::getLogType(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
	{
		switch (messageSeverity)
		{
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
			return LogType::LOG_TYPE_DEBUG;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
			return LogType::LOG_TYPE_INFO;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
			return LogType::LOG_TYPE_WARNING;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
			return LogType::LOG_TYPE_ERROR;
		case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT:
			return LogType::LOG_TYPE_INFO;
		}

		return LogType::LOG_TYPE_INFO;
	}

	void VulkanRenderer::createDebugManager()
	{
		if (!validationLayersAreEnabled())
		{
			return;
		}
		
		VkDebugUtilsBuilder()
			.setDebugCallback(debugCallback)
			.build(storage);
	}

	void VulkanRenderer::createSurface()
	{
		VkSurfaceBuilder::build(storage);
	}

	void VulkanRenderer::createDevices()
	{
		VkDeviceBuilder::build(storage);
	}

	void VulkanRenderer::createQueues()
	{
		VkQueuesBuilder::build(storage);
	}

	void VulkanRenderer::createSwapChain()
	{
		VkSwapChainBuilder::build(storage);
	}

	std::optional<uint32_t> VulkanRenderer::acquireNextImage()
	{
		ASSERT(static_cast<size_t>(currentFrame) < imageAvailableSemaphores.size() && currentFrame >= 0,
			"Current frame number is larger than the number of fences.");

		uint32_t imageIndex;
		const VkResult result = vkAcquireNextImageKHR(storage.logicalDevice, storage.swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreateSwapChain();
			return std::nullopt;
		}

		ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "failed to acquire swap chain image.");

		return imageIndex;
	}

	void VulkanRenderer::recreateSwapChain()
	{
		Services::get<imgui::ImGuiLibrary>()->handleMinimization();
		Services::get<VulkanRenderer>()->deviceWaitIdle();

		cleanupSwapChain();

		createSwapChain();
		createColorResources();
		createDepthResources();
		createFramebuffers();
	}

	void VulkanRenderer::cleanupSwapChain() const
	{
		vkDestroyImageView(storage.logicalDevice, depthTexture.imageView, nullptr);
		vkDestroyImage(storage.logicalDevice, depthTexture.image, nullptr);
		vkFreeMemory(storage.logicalDevice, depthTexture.imageMemory, nullptr);

		vkDestroyImageView(storage.logicalDevice, colorTexture.imageView, nullptr);
		vkDestroyImage(storage.logicalDevice, colorTexture.image, nullptr);
		vkFreeMemory(storage.logicalDevice, colorTexture.imageMemory, nullptr);

		for (const auto framebuffer : swapChainFramebuffers) {
			vkDestroyFramebuffer(storage.logicalDevice, framebuffer, nullptr);
		}

		for (const auto imageView : storage.swapChainDetails.swapChainImageViews) {
			vkDestroyImageView(storage.logicalDevice, imageView, nullptr);
		}

		vkDestroySwapchainKHR(storage.logicalDevice, storage.swapChain, nullptr);
	}

	void VulkanRenderer::createRenderPass()
	{
		VkRenderPassBuilder::build("Main Render Pass", storage);
	}

	void VulkanRenderer::createDescriptorSetLayout()
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

	void VulkanRenderer::createDescriptorPool()
	{
		VkDescriptorPoolBuilder()
			.setMaxSets(MAX_FRAMES_IN_FLIGHT * MAX_NUMBER_OF_MESHES * NUMBER_OF_TEXTURE_TYPES + IMAGE_SAMPLER_POOL_SIZE + 1)
			.setPoolSizes({{
				// Descriptor pool for global descriptor set (set = 0)
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = MAX_FRAMES_IN_FLIGHT
				},
				// Descriptor pool for instance descriptor set (set = 1)
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = MAX_FRAMES_IN_FLIGHT * MAX_NUMBER_OF_MESHES * 2
				},
				// Descriptor pool for material descriptor set (set = 2)
				{
					.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.descriptorCount = MAX_NUMBER_OF_MESHES * NUMBER_OF_TEXTURE_TYPES + IMAGE_SAMPLER_POOL_SIZE
				},
				// Descriptor pool for light descriptor set (set = 3)
				{
					.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.descriptorCount = MAX_FRAMES_IN_FLIGHT
				},
			}})
			.build(storage);
	}

	void VulkanRenderer::createCubemapTexture()
	{
		const VkImage cubemapImage = VkImageBuilder("Cubemap Image")
			.setWidth(512)
			.setHeight(512)
			.setArrayLayers(6)
			.setFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
			.setUsage(VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.setFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
			.build(storage);
		
		cubemap.image = cubemapImage;

		cubemap.imageMemory = VkDeviceMemoryBuilder("Cubemap Image Memory")
			.setImage(cubemap.image)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		const VkImageView cubemapImageView = VkImageViewBuilder()
			.setImage(cubemapImage)
			.setViewType(VK_IMAGE_VIEW_TYPE_CUBE)
			.setFormat(VK_FORMAT_R32G32B32A32_SFLOAT)
			.setComponents({VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A})
			.setAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.setLayerCount(6)
			.build("Cubemap Image View", storage);
		
		cubemap.imageView = cubemapImageView;
		
		cubemap.sampler = VkSamplerBuilder("Cubemap Sampler")
			.setSamplerMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
			.setBorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
			.build(storage);
		
		Services::get<World>()->getStorage()->setCubemapTexture(std::make_shared<VulkanTexture2d>(cubemap));
	}
	
	void VulkanRenderer::createSkyPipeline()
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

	void VulkanRenderer::createGraphicsPipeline()
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

	VkShaderModule VulkanRenderer::createShaderModule(const std::vector<char>& code, const VkDevice& device)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		ASSERT(vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS, "failed to create shader module.");

		return shaderModule;
	}

	void VulkanRenderer::createFramebuffers()
	{
		swapChainFramebuffers.resize(storage.swapChainDetails.swapChainImageViews.size());

		for (size_t i = 0; i < storage.swapChainDetails.swapChainImageViews.size(); ++i)
		{
			std::array attachments = {
				colorTexture.imageView,
				depthTexture.imageView,
				storage.swapChainDetails.swapChainImageViews[i]
			};

			const auto& [width, height] = storage.swapChainDetails.swapChainExtent;

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = storage.renderPass;
			framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebufferInfo.pAttachments = attachments.data();
			framebufferInfo.width = width;
			framebufferInfo.height = height;
			framebufferInfo.layers = 1;

			ASSERT(vkCreateFramebuffer(storage.logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) == VK_SUCCESS, "failed to create framebuffer.");
		}
	}

	void VulkanRenderer::createCommandBuffer()
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = getCommandPool();
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

		ASSERT(vkAllocateCommandBuffers(storage.logicalDevice, &allocInfo, commandBuffers.data()) == VK_SUCCESS, "failed to allocate command buffers.");
	}

	void VulkanRenderer::resetCommandBuffer(const int bufferId) const
	{
		ASSERT(static_cast<size_t>(bufferId) < commandBuffers.size() && bufferId >= 0, "current frame number is larger than number of fences.");

		ASSERT(vkResetCommandBuffer(commandBuffers[bufferId], 0) == VK_SUCCESS, "Failed to reset command buffer.");
	}

	void VulkanRenderer::drawMainScenePass(const VkCommandBuffer commandBufferToRecord) const
	{
		if (!globalBuffers.vertexBuffer)
		{
			return;
		}
		
		// ==== [ MAIN SCENE PASS ] ====
		vkCmdBindPipeline(commandBufferToRecord, VK_PIPELINE_BIND_POINT_GRAPHICS, storage.mainPipeline);
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(storage.swapChainDetails.swapChainExtent.width);
		viewport.height = static_cast<float>(storage.swapChainDetails.swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBufferToRecord, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { .x = 0, .y = 0 };
		scissor.extent = storage.swapChainDetails.swapChainExtent;
		vkCmdSetScissor(commandBufferToRecord, 0, 1, &scissor);

		const VkBuffer vertexBuffers[] = { globalBuffers.vertexBuffer };
		constexpr VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBufferToRecord, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBufferToRecord, globalBuffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Bind the global descriptor set.
		vkCmdBindDescriptorSets(
			commandBufferToRecord,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			storage.mainPipelineLayout,
			0,
			1,
			&globalDescriptorSets[currentFrame], 0, nullptr);

		// Bind the directional light descriptor set.
		vkCmdBindDescriptorSets(
			commandBufferToRecord,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			storage.mainPipelineLayout,
			3,
			1,
			&directionalLight.descriptorSets[currentFrame], 0, nullptr);
		
		for (const auto& meshInstance : meshInstances)
		{
			// Bind the instance descriptor set.
			vkCmdBindDescriptorSets(
				commandBufferToRecord,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				storage.mainPipelineLayout,
				1,
				1,
				&meshInstance.instanceDescriptorSets[currentFrame], 0, nullptr);

			
			for (const auto& meshPart : meshInstance.mesh->meshParts)
			{
				// Bind the material descriptor set.
				vkCmdBindDescriptorSets(
					commandBufferToRecord,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					storage.mainPipelineLayout,
					2,
					1,
					&meshPart.material->materialDescriptorSet, 0, nullptr);

				// Draw mesh part.
				vkCmdDrawIndexed(commandBufferToRecord,
	                 static_cast<uint32_t>(meshPart.indexCount),
	                 1,
	                 static_cast<uint32_t>(meshPart.indexOffset),
	                 static_cast<int32_t>(meshPart.vertexOffset),
	                 0);
			}
		}
	}

	void VulkanRenderer::recordCommandBuffer(const VkCommandBuffer commandBufferToRecord, const uint32_t imageIndex) const
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		ASSERT(vkBeginCommandBuffer(commandBufferToRecord, &beginInfo) == VK_SUCCESS,
			"Failed to begin recording command buffer.");

		// Start the rendering pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = storage.renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { .x = 0, .y = 0};
		renderPassInfo.renderArea.extent = storage.swapChainDetails.swapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { .depth = 1.0f, .stencil = 0};

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// Draw.
		vkCmdBeginRenderPass(commandBufferToRecord, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			drawSkyboxPass(commandBufferToRecord);
			drawMainScenePass(commandBufferToRecord);
			Services::get<imgui::ImGuiLibrary>()->renderDrawData(commandBufferToRecord);
		vkCmdEndRenderPass(commandBufferToRecord);
		
		ASSERT(vkEndCommandBuffer(commandBufferToRecord) == VK_SUCCESS, "Failed to fend recording command buffer.");
	}

	void VulkanRenderer::drawSkyboxPass(const VkCommandBuffer commandBufferToRecord) const
	{
		// ==== [ SKYBOX PASS ] ====
		ASSERT(!Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::SKY).empty(),
			   "Sky mesh must always exist");

		const std::shared_ptr<Mesh> skyMesh = Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::SKY)[0];
		
		ASSERT(!skyMesh->meshParts.empty(),
			   "Sky mesh exists, but it has zero mesh parts.");

		const MeshPart& skyMeshPart = skyMesh->meshParts[0];
		
		ASSERT(skyMeshPart.vertexCount > 0 && skyMeshPart.indexCount > 0,
			   "Sky mesh has no vertices or indices");
		
		vkCmdBindPipeline(commandBufferToRecord, VK_PIPELINE_BIND_POINT_GRAPHICS, storage.skyPipeline);
		
		// Viewport and scissor
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(storage.swapChainDetails.swapChainExtent.width);
		viewport.height = static_cast<float>(storage.swapChainDetails.swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBufferToRecord, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { .x = 0, .y = 0 };
		scissor.extent = storage.swapChainDetails.swapChainExtent;
		vkCmdSetScissor(commandBufferToRecord, 0, 1, &scissor);

		// Sky vertices
		const VkBuffer skyVertexBuffers[] = { globalBuffers.skyVertexBuffer };
		constexpr VkDeviceSize skyOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBufferToRecord, 0, 1, skyVertexBuffers, skyOffsets);
		vkCmdBindIndexBuffer(commandBufferToRecord, globalBuffers.skyIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Bind
		vkCmdBindDescriptorSets(
			commandBufferToRecord,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			storage.skyPipelineLayout,
			0, 1,
			&globalDescriptorSets[currentFrame],
			0, nullptr);
		
		// Draw mesh part.
		vkCmdDrawIndexed(commandBufferToRecord,
			 static_cast<uint32_t>(skyMeshPart.indexCount),
			 1,
			 static_cast<uint32_t>(skyMeshPart.indexOffset),
			 static_cast<int32_t>(skyMeshPart.vertexOffset),
			 0);
	}

	VkCommandBuffer VulkanRenderer::getCommandBuffer(const int bufferId) const
	{
		ASSERT(static_cast<size_t>(bufferId) < commandBuffers.size() && bufferId >= 0, "current frame number is larger than number of fences.");

		return commandBuffers[bufferId];
	}

	VkCommandPool VulkanRenderer::getCommandPool()
	{
		const std::thread::id threadId = std::this_thread::get_id();
		if (!threadCommandPools.contains(threadId))
		{
			threadCommandPools[threadId] = createCommandPool();		
		}
		
		return threadCommandPools[threadId];
	}
	
	VkCommandPool VulkanRenderer::createCommandPool() const
	{
		VkCommandPool newCommandPool;
		
		const auto [graphicsFamily, presentFamily] = utils::findQueueFamilies(storage.physicalDevice, storage.surface);
		ASSERT(graphicsFamily.has_value(), "Graphics family is incomplete.");
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = graphicsFamily.value();

		ASSERT(vkCreateCommandPool(storage.logicalDevice, &poolInfo, nullptr, &newCommandPool) == VK_SUCCESS, "failed to create command pool.");
		return newCommandPool;
	}

	VkCommandBuffer VulkanRenderer::beginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = getCommandPool();
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(storage.logicalDevice, &allocInfo, &commandBuffer);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(commandBuffer, &beginInfo);

		return commandBuffer;
	}

	void VulkanRenderer::endSingleTimeCommands(const VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		utils::threadSafeQueueSubmit(storage, &submitInfo, nullptr);
		
		{
			std::scoped_lock lock(storage.graphicsQueueMutex);
			vkQueueWaitIdle(storage.graphicsQueue);
		}
		
		vkFreeCommandBuffers(storage.logicalDevice, getCommandPool(), 1, &commandBuffer);
	}

	void VulkanRenderer::createDepthResources()
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
	
	bool VulkanRenderer::hasStencilComponent(const VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void VulkanRenderer::generateMipmaps(const VulkanTexture2d& texture, const VkFormat imageFormat, const int32_t texWidth, const int32_t texHeight)
	{
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(storage.physicalDevice, imageFormat, &formatProperties);

		ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT, 
			"Texture image format does not support linear blitting.");

		const VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = texture.image;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;
		barrier.subresourceRange.levelCount = 1;

		int32_t mipWidth = texWidth;
		int32_t mipHeight = texHeight;

		for (uint32_t i = 1; i < texture.maxMipLevels; i++) {
			barrier.subresourceRange.baseMipLevel = i - 1;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			VkImageBlit blit{};
			blit.srcOffsets[0] = { 0, 0, 0 };
			blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
			blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.srcSubresource.mipLevel = i - 1;
			blit.srcSubresource.baseArrayLayer = 0;
			blit.srcSubresource.layerCount = 1;
			blit.dstOffsets[0] = { 0, 0, 0 };
			blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
			blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			blit.dstSubresource.mipLevel = i;
			blit.dstSubresource.baseArrayLayer = 0;
			blit.dstSubresource.layerCount = 1;

			vkCmdBlitImage(commandBuffer,
				texture.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				texture.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

			if (mipWidth > 1) mipWidth /= 2;
			if (mipHeight > 1) mipHeight /= 2;
		}

		barrier.subresourceRange.baseMipLevel = texture.maxMipLevels - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

		endSingleTimeCommands(commandBuffer);
	}

	void VulkanRenderer::transitionImageLayout(
		const VkImage image,
		[[maybe_unused]] VkFormat format,
		const VkImageLayout oldLayout,
		const VkImageLayout newLayout,
		const uint32_t mipLevels)
	{
		const VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = mipLevels;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		VkPipelineStageFlags sourceStage{};
		VkPipelineStageFlags destinationStage{};

		ASSERT((oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			|| (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL),
			"unsupported layout transition.");

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		endSingleTimeCommands(commandBuffer);
	}

	void VulkanRenderer::copyBufferToImage(const VkBuffer buffer, const VkImage image, const uint32_t width, const uint32_t height)
	{
		const VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = { 0, 0, 0 };
		region.imageExtent = {
			width,
			height,
			1
		};

		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

		endSingleTimeCommands(commandBuffer);
	}

	void VulkanRenderer::createColorResources()
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

	void VulkanRenderer::createVertexBuffer(const std::vector<math::Vertex>& vertices)
	{
		if (globalBuffers.vertexBuffer != VK_NULL_HANDLE)
		{
			frames[currentFrame].buffersToDelete.emplace_back(
			   globalBuffers.vertexBuffer, 
			   globalBuffers.vertexBufferMemory
			);
		}
		
		const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		// Fill vertex buffer data.
		void* data;
		vkMapMemory(storage.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), bufferSize);
		vkUnmapMemory(storage.logicalDevice, stagingBufferMemory);

		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			globalBuffers.vertexBuffer,
			globalBuffers.vertexBufferMemory);
		
		copyBuffer(stagingBuffer, globalBuffers.vertexBuffer, bufferSize);
		vkDestroyBuffer(storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, stagingBufferMemory, nullptr);
	}

	void VulkanRenderer::createSkyVertexBuffer(const std::vector<math::Vertex>& vertices)
	{
		if (globalBuffers.skyVertexBuffer != VK_NULL_HANDLE)
		{
			frames[currentFrame].buffersToDelete.emplace_back(
			   globalBuffers.skyVertexBuffer, 
			   globalBuffers.skyVertexBufferMemory
			);
		}
		
		const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		// Fill vertex buffer data.
		void* data;
		vkMapMemory(storage.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), bufferSize);
		vkUnmapMemory(storage.logicalDevice, stagingBufferMemory);

		createBuffer(
			bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			globalBuffers.skyVertexBuffer,
			globalBuffers.skyVertexBufferMemory);
		
		copyBuffer(stagingBuffer, globalBuffers.skyVertexBuffer, bufferSize);
		vkDestroyBuffer(storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, stagingBufferMemory, nullptr);
	}

	void VulkanRenderer::createBuffer(const VkDeviceSize size, const VkBufferUsageFlags usage, const VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) const
	{
		ASSERT(size > 0, "Buffer size must not be empty");
		
		// Create buffer structure.
		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = size;
		bufferInfo.usage = usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		ASSERT(vkCreateBuffer(storage.logicalDevice, &bufferInfo, nullptr, &buffer) == VK_SUCCESS,
			"Failed to create buffer.");
		
		ASSERT(buffer != VK_NULL_HANDLE,
			"Buffer must be valid.");

		// Calculate memory requirements.
		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(storage.logicalDevice, buffer, &memRequirements);

		// Allocate vertex buffer memory.
		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = utils::findMemoryType(storage, memRequirements.memoryTypeBits, properties);

		ASSERT(vkAllocateMemory(storage.logicalDevice, &allocInfo, nullptr, &bufferMemory) == VK_SUCCESS,
			"Failed to allocate buffer memory");

		ASSERT(vkBindBufferMemory(storage.logicalDevice, buffer, bufferMemory, 0) == VK_SUCCESS,
			"Failed to bind buffer memory");
	}

	void VulkanRenderer::copyBuffer(const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size)
	{
		const VkCommandBuffer commandBuffer = beginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		endSingleTimeCommands(commandBuffer);
	}

	void VulkanRenderer::createSkyIndexBuffer(const std::vector<uint32_t>& indices)
	{
		if (globalBuffers.skyIndexBuffer != VK_NULL_HANDLE)
		{
			frames[currentFrame].buffersToDelete.emplace_back(
			   globalBuffers.skyIndexBuffer, 
			   globalBuffers.skyIndexBufferMemory
			);
		}
		
		const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(storage.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), bufferSize);
		vkUnmapMemory(storage.logicalDevice, stagingBufferMemory);

		createBuffer(bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			globalBuffers.skyIndexBuffer,
			globalBuffers.skyIndexBufferMemory);

		copyBuffer(stagingBuffer, globalBuffers.skyIndexBuffer, bufferSize);

		vkDestroyBuffer(storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, stagingBufferMemory, nullptr);
	}

	void VulkanRenderer::createIndexBuffer(const std::vector<uint32_t>& indices)
	{
		if (globalBuffers.indexBuffer != VK_NULL_HANDLE)
		{
			frames[currentFrame].buffersToDelete.emplace_back(
			   globalBuffers.indexBuffer, 
			   globalBuffers.indexBufferMemory
			);
		}
		
		const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;
		createBuffer(bufferSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer,
			stagingBufferMemory);

		void* data;
		vkMapMemory(storage.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(storage.logicalDevice, stagingBufferMemory);

		createBuffer(bufferSize,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			globalBuffers.indexBuffer,
			globalBuffers.indexBufferMemory);

		copyBuffer(stagingBuffer, globalBuffers.indexBuffer, bufferSize);

		vkDestroyBuffer(storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, stagingBufferMemory, nullptr);
	}

	void VulkanRenderer::createUniformBuffer()
	{
		// Global UBO
		constexpr VkDeviceSize globalUboSize = sizeof(math::GlobalUbo);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			createBuffer(
				globalUboSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				globalUboBuffer.frameBuffers[i],
				globalUboBuffer.memory[i]);

			ASSERT(globalUboBuffer.frameBuffers[i] != VK_NULL_HANDLE, "Global Buffer must be valid");
			vkMapMemory(
				storage.logicalDevice,
				globalUboBuffer.memory[i],
				0,
				globalUboSize,
				0,
				&globalUboBuffer.mapped[i]);
		}

		ASSERT(globalUboBuffer.frameBuffers[0] != VK_NULL_HANDLE, "Global Buffer must be invalid");
		
		// Instance UBO
		constexpr VkDeviceSize instanceUboSize = sizeof(math::InstanceUbo);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			createBuffer(instanceUboSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				instanceUboBuffer.frameBuffers[i],
				instanceUboBuffer.memory[i]);

			ASSERT(instanceUboBuffer.frameBuffers[i] != VK_NULL_HANDLE, "Instance Buffer must be valid");

			vkMapMemory(storage.logicalDevice, instanceUboBuffer.memory[i], 0, instanceUboSize, 0, &instanceUboBuffer.mapped[i]);
		}

		ASSERT(instanceUboBuffer.frameBuffers[0] != VK_NULL_HANDLE, "Instance Buffer must be valid");

		// Directional Light UBO
		constexpr VkDeviceSize lightUboSize = sizeof(math::DirectionalLightUbo);
		
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			createBuffer(lightUboSize,
				VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
				directionalLightUboBuffer.frameBuffers[i],
				directionalLightUboBuffer.memory[i]);

			ASSERT(directionalLightUboBuffer.frameBuffers[i] != VK_NULL_HANDLE, "Directional Light Buffer must be valid");

			vkMapMemory(storage.logicalDevice, directionalLightUboBuffer.memory[i], 0, lightUboSize, 0, &directionalLightUboBuffer.mapped[i]);
		}
	
	}

	void VulkanRenderer::updateUniformBuffer(const uint32_t currentImage)
	{
		const SpectatorCamera camera = Services::get<World>()->getMainCamera();

		// Global UBO
		math::GlobalUbo globalUbo{};

		globalUbo.view = math::Matrix4x4::lookAt(
			camera.getPosition(),
			camera.getPosition() + camera.getForwardVector(),
			camera.getUpVector()).trivial();

		globalUbo.projection = math::Matrix4x4::perspective(
			math::radians(45.0f),
			static_cast<float>(storage.swapChainDetails.swapChainExtent.width) / static_cast<float>(storage.swapChainDetails.swapChainExtent.height),
			Z_NEAR, Z_FAR).trivial();

		globalUbo.cameraPosition = camera.getPosition().trivial();

		globalUbo.debug = isDrawDebugEnabled ? 1 : 0;
		
		memcpy(globalUboBuffer.mapped[currentImage], &globalUbo, sizeof(globalUbo));

		// Instance UBO
		math::InstanceUbo instanceUbo{};
		instanceUbo.model = math::Matrix4x4().trivial();

		memcpy(instanceUboBuffer.mapped[currentImage], &instanceUbo, sizeof(instanceUbo));

		// Directional Light UBO
		math::DirectionalLightUbo directionalLightUbo{};
		directionalLightUbo.color = directionalLight.light.color;
		directionalLightUbo.direction = directionalLight.light.direction;
		
		memcpy(directionalLightUboBuffer.mapped[currentFrame], &directionalLightUbo, sizeof(directionalLightUbo));
	}

	void VulkanRenderer::createMeshDescriptorSets(const std::shared_ptr<Mesh>& mesh)
	{
		createGlobalDescriptorSets();
		createInstanceDescriptorSets();
		createMaterialDescriptorSets(mesh);
	}

	void VulkanRenderer::createGlobalDescriptorSets()
	{
		const std::vector globalLayouts(MAX_FRAMES_IN_FLIGHT, storage.globalDescriptorSetLayout);
		VkDescriptorSetAllocateInfo globalAllocateInfo{};
		globalAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		globalAllocateInfo.descriptorPool = storage.descriptorPool;
		globalAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		globalAllocateInfo.pSetLayouts = globalLayouts.data();

		ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &globalAllocateInfo, globalDescriptorSets.data()) == VK_SUCCESS,
			"Failed to allocate global descriptor sets.");

		for (size_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; frameIndex++)
		{
			// Global UBO descriptor set
			const VkDescriptorBufferInfo globalBufferInfo =
				{
					.buffer = globalUboBuffer.frameBuffers[frameIndex],
					.offset = 0,
					.range = sizeof(math::GlobalUbo)
				};

			const VkWriteDescriptorSet globalWrite =
				{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = globalDescriptorSets[frameIndex],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pImageInfo = nullptr,
				.pBufferInfo = &globalBufferInfo,
				.pTexelBufferView = nullptr
			};

			vkUpdateDescriptorSets(storage.logicalDevice, 1, &globalWrite, 0, nullptr);
		}
	}
	
	void VulkanRenderer::createInstanceDescriptorSets()
	{
		for (auto& meshInstance : meshInstances)
		{
			const std::vector instanceLayouts(MAX_FRAMES_IN_FLIGHT, storage.instanceDescriptorSetLayout);
			VkDescriptorSetAllocateInfo instanceSetAllocateInfo{};
			instanceSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			instanceSetAllocateInfo.descriptorPool = storage.descriptorPool;
			instanceSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
			instanceSetAllocateInfo.pSetLayouts = instanceLayouts.data();

			meshInstance.instanceDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
			ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &instanceSetAllocateInfo, meshInstance.instanceDescriptorSets.data()) == VK_SUCCESS,
				"Failed to allocate instance descriptor sets.");

			for (size_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; frameIndex++)
			{
				// Instance UBO descriptor set
				const VkDescriptorBufferInfo instanceBufferInfo =
					{
						.buffer = instanceUboBuffer.frameBuffers[frameIndex],
						.offset = 0,
						.range = sizeof(math::InstanceUbo)
					};

				const VkWriteDescriptorSet descriptorWrite =
					{
						.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
						.pNext = nullptr,
						.dstSet = meshInstance.instanceDescriptorSets[frameIndex],
						.dstBinding = 0,
						.dstArrayElement = 0,
						.descriptorCount = 1,
						.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
						.pImageInfo = nullptr,
						.pBufferInfo = &instanceBufferInfo,
						.pTexelBufferView = nullptr
					};

				vkUpdateDescriptorSets(storage.logicalDevice, 1, &descriptorWrite, 0, nullptr);
			}
		}
	}

	void VulkanRenderer::createMaterialDescriptorSets(const std::shared_ptr<Mesh>& mesh) const
	{
		for (auto& meshPart : mesh->meshParts)
		{
			// Material descriptor set
			ASSERT(meshPart.material, "Material must exist for any mesh part.");
			const auto& material = meshPart.material;
			
			VkDescriptorSetAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = storage.descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &storage.materialDescriptorSetLayout;

			ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &allocInfo, &material->materialDescriptorSet) == VK_SUCCESS,
				"Failed to allocate material descriptor sets.");
        
			std::vector<VkDescriptorImageInfo> imageInfos;
			imageInfos.reserve(NUMBER_OF_TEXTURE_TYPES);

			material->iterateAllTextures([&]([[maybe_unused]] const TextureType textureType, const std::shared_ptr<const VulkanTexture2d>& texture)
			{
				imageInfos.push_back(
					{
						.sampler = texture->sampler,
						.imageView = texture->imageView,
						.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					});
			});

			std::vector<VkWriteDescriptorSet> descriptorWrites;
			descriptorWrites.reserve(imageInfos.size());
			for (uint32_t i = 0; i < imageInfos.size(); ++i)
			{
				descriptorWrites.push_back({
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = material->materialDescriptorSet,
					.dstBinding = i,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &imageInfos[i],
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				});
			}

			vkUpdateDescriptorSets(storage.logicalDevice,
				static_cast<uint32_t>(descriptorWrites.size()),
				descriptorWrites.data(),
				0, nullptr);
		}
	}

	void VulkanRenderer::createLightsDescriptorSets()
	{
		if (!directionalLight.descriptorSets.empty())
		{
			return;
		}
		
		const std::vector lightLayouts(MAX_FRAMES_IN_FLIGHT, storage.lightsDescriptorSetLayout);
		VkDescriptorSetAllocateInfo lightSetAllocateInfo{};
		lightSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		lightSetAllocateInfo.descriptorPool = storage.descriptorPool;
		lightSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		lightSetAllocateInfo.pSetLayouts = lightLayouts.data();

		directionalLight.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &lightSetAllocateInfo, directionalLight.descriptorSets.data()) == VK_SUCCESS,
			"Failed to allocate instance descriptor sets.");

		for (size_t frameIndex = 0; frameIndex < MAX_FRAMES_IN_FLIGHT; frameIndex++)
		{
			std::array<VkWriteDescriptorSet, 2> descriptorWrites;

			// Light UBO descriptor set
			const VkDescriptorBufferInfo directionalLightBufferInfo =
				{
					.buffer = directionalLightUboBuffer.frameBuffers[frameIndex],
					.offset = 0,
					.range = sizeof(math::DirectionalLightUbo)
				};

			descriptorWrites[0] =
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = directionalLight.descriptorSets[frameIndex],
					.dstBinding = 0,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
					.pImageInfo = nullptr,
					.pBufferInfo = &directionalLightBufferInfo,
					.pTexelBufferView = nullptr
				};

			VkDescriptorImageInfo descriptorImageInfo = {};
			descriptorImageInfo.imageView = cubemap.imageView;
			descriptorImageInfo.sampler = cubemap.sampler;
			descriptorImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			descriptorWrites[1] =
				{
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.pNext = nullptr,
					.dstSet = directionalLight.descriptorSets[frameIndex],
					.dstBinding = 1,
					.dstArrayElement = 0,
					.descriptorCount = 1,
					.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					.pImageInfo = &descriptorImageInfo,
					.pBufferInfo = nullptr,
					.pTexelBufferView = nullptr
				};
			
			vkUpdateDescriptorSets(storage.logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void VulkanRenderer::createSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			ASSERT(vkCreateSemaphore(storage.logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) == VK_SUCCESS &&
				vkCreateSemaphore(storage.logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) == VK_SUCCESS &&
				vkCreateFence(storage.logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]) == VK_SUCCESS,
				"failed to create semaphores.");
		}
	}

	void VulkanRenderer::waitForFences() const
	{
		ASSERT(static_cast<size_t>(currentFrame) < inFlightFences.size() && currentFrame >= 0, "current frame number is larger than number of fences.");

		vkWaitForFences(storage.logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
		vkResetFences(storage.logicalDevice, 1, &inFlightFences[currentFrame]);
	}

	void VulkanRenderer::resetFences() const
	{
		ASSERT(static_cast<size_t>(currentFrame) < inFlightFences.size() && currentFrame >= 0, "current frame number is larger than number of fences.");

		vkResetFences(storage.logicalDevice, 1, &inFlightFences[currentFrame]);
	}

	void VulkanRenderer::onResize()
	{
		framebufferResized = true;
	}

	VkSampleCountFlagBits VulkanRenderer::getMaxUsableSampleCount() const
	{
		VkPhysicalDeviceProperties physicalDeviceProperties;
		vkGetPhysicalDeviceProperties(storage.physicalDevice, &physicalDeviceProperties);

		const VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
		if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
		if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
		if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
		if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
		if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
		if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

		return VK_SAMPLE_COUNT_1_BIT;
	}
	

	void VulkanRenderer::cleanupFrameResources()
	{
		// Destroy buffers scheduled for deletion from N frames ago
		const uint32_t frameToClean = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    
		for (auto& [buffer, memory] : frames[frameToClean].buffersToDelete)
		{
			vkDestroyBuffer(storage.logicalDevice, buffer, nullptr);
			vkFreeMemory(storage.logicalDevice, memory, nullptr);
		}
		frames[frameToClean].buffersToDelete.clear();
	}
	
}
