#include "VulkanRenderer.h"
#include "pass/VulkanRenderPass.h"

// #define SAVE_CAPTURE_CUBEMAP_TEXTURES

#ifdef SAVE_CAPTURE_CUBEMAP_TEXTURES
#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable: 4996)
#include "third-party/stb_image_write.h"
#pragma warning(pop)
#endif // SAVE_CAPTURE_CUBEMAP_TEXTURES

#include <array>
#include <chrono>
#include <set>
#include <stdexcept>

#include "builder/VkBufferBuilder.h"
#include "builder/VkPipelineBuilder.h"
#include <filesystem>
#include "builder/VkDeviceMemoryBuilder.h"
#include "builder/VkImageBuilder.h"
#include "builder/VkImageViewBuilder.h"
#include "builder/VkSamplerBuilder.h"
#include "engine/input/Input.h"
#include "services/platform/Platform.h"
#include "engine/Event.h"
#include "engine/utils/Utils.h"
#include "services/graphics/GraphicsLibrary.h"
#include "services/renderer/vulkan/GraphicsOverlay.h"
#include "engine/utils/math/UniformBufferObjects.h"
#include "material/VulkanMaterial.h"
#include "services/Services.h"
#include "services/threading/ThreadPool.h"
#include "services/world/World.h"
#include "utils/VulkanUtils.h"


namespace parus::vulkan
{
	void VulkanRenderer::init()
	{
		registerEvents();
		defineDescriptors();
		initializer.initialize(storage, descriptorManager, configurator);
		shadowPass.init(storage, configurator);
		shadowPass.initPipeline(storage, descriptorManager, configurator);
		depthPrePass.init(storage, configurator);
		depthPrePass.initPipeline(storage, descriptorManager);
		ssaoPass.init(storage, configurator);
		ssaoPass.initPipeline(storage, descriptorManager, depthPrePass);
		ssaoBlurPass.init(storage, configurator);
		ssaoBlurPass.initPipeline(storage, depthPrePass, ssaoPass);
		mainPass.init(storage, configurator);
		mainPass.initPipelines(storage, descriptorManager);
		auto* graphicsOverlay = dynamic_cast<GraphicsOverlay*>(Services::get<GraphicsLibrary>().get());
		mainPass.setOverlay(graphicsOverlay);
		loadSceneAssets();
		isRunning = true;
	}
	
	const VulkanStorage& VulkanRenderer::getStorage() const
	{
		return storage;
	}

	void VulkanRenderer::registerEvents()
	{
		REGISTER_EVENT(EventType::EVENT_WINDOW_RESIZED, [&](const int newWidth, const int newHeight)
		{
			LOG_INFO("Vulkan initiated window resize. New dimensions: " + std::to_string(newWidth) + " " + std::to_string(newHeight));
			onResize();
		});

		REGISTER_EVENT(EventType::EVENT_KEY_PRESSED, [&](const KeyButton key)
		{
			if (key == KeyButton::KEY_Z && !Services::get<GraphicsLibrary>()->isCapturingInput())
			{
				debugMode = (debugMode + 1) % 10;
				LOG_DEBUG("Debug mode: " + std::to_string(debugMode));
			}
		});

		REGISTER_EVENT(EventType::EVENT_APPLICATION_QUIT, [&]([[maybe_unused]]const int exitCode)
		{
			isRunning = false;
		});
	}

	void VulkanRenderer::defineDescriptors()
	{
		using DescriptorType = VulkanDescriptorManager::DescriptorType;
		constexpr size_t MAX_MESHES = 100;
		constexpr uint32_t IMAGE_SAMPLER_POOL = 1000;

		descriptorManager.define(DescriptorType::GLOBAL,
			VulkanDescriptor()
				.withLayout("Descriptor Set 0 - Global UBO",
					{{ "Binding 0: Global UBO", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }})
				.withPoolSizes(
					{{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = VulkanStorage::MAX_FRAMES_IN_FLIGHT }},
					VulkanStorage::MAX_FRAMES_IN_FLIGHT)
				.withAllocator([&](VulkanStorage& s, const VkDescriptorSetLayout layout)
				{
					if (s.globalDescriptorSets[0] != VK_NULL_HANDLE)
					{
						return;
					}

					const std::vector globalLayouts(VulkanStorage::MAX_FRAMES_IN_FLIGHT, layout);
					VkDescriptorSetAllocateInfo allocInfo{};
					allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					allocInfo.descriptorPool = s.descriptorPool;
					allocInfo.descriptorSetCount = static_cast<uint32_t>(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
					allocInfo.pSetLayouts = globalLayouts.data();

					ASSERT(vkAllocateDescriptorSets(s.logicalDevice, &allocInfo, s.globalDescriptorSets.data()) == VK_SUCCESS,
						"Failed to allocate global descriptor sets.");

					for (size_t i = 0; i < VulkanStorage::MAX_FRAMES_IN_FLIGHT; i++)
					{
						const VkDescriptorBufferInfo bufferInfo = { .buffer = s.globalUboBuffer.frameBuffers[i], .offset = 0, .range = sizeof(math::GlobalUbo) };
						const VkWriteDescriptorSet write = {
							.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr,
							.dstSet = s.globalDescriptorSets[i], .dstBinding = 0, .dstArrayElement = 0,
							.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
							.pImageInfo = nullptr, .pBufferInfo = &bufferInfo, .pTexelBufferView = nullptr
						};
						vkUpdateDescriptorSets(s.logicalDevice, 1, &write, 0, nullptr);
					}
				}));

		descriptorManager.define(DescriptorType::INSTANCE,
			VulkanDescriptor()
				.withLayout("Descriptor Set 1 - Instance UBO",
					{{ "Binding 0: Instance UBO", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT }})
				.withPoolSizes(
					{{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, .descriptorCount = VulkanStorage::MAX_FRAMES_IN_FLIGHT * static_cast<uint32_t>(MAX_MESHES) * 2 }},
					VulkanStorage::MAX_FRAMES_IN_FLIGHT * static_cast<uint32_t>(MAX_MESHES))
				.withAllocator([&](VulkanStorage& s, const VkDescriptorSetLayout layout)
				{
					for (auto& meshInstance : meshInstances)
					{
						if (!meshInstance.instanceDescriptorSets.empty())
						{
							continue;
						}

						const std::vector instanceLayouts(VulkanStorage::MAX_FRAMES_IN_FLIGHT, layout);
						VkDescriptorSetAllocateInfo allocInfo{};
						allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
						allocInfo.descriptorPool = s.descriptorPool;
						allocInfo.descriptorSetCount = static_cast<uint32_t>(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
						allocInfo.pSetLayouts = instanceLayouts.data();

						meshInstance.instanceDescriptorSets.resize(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
						ASSERT(vkAllocateDescriptorSets(s.logicalDevice, &allocInfo, meshInstance.instanceDescriptorSets.data()) == VK_SUCCESS,
							"Failed to allocate instance descriptor sets.");

						for (size_t i = 0; i < VulkanStorage::MAX_FRAMES_IN_FLIGHT; i++)
						{
							const VkDescriptorBufferInfo bufferInfo = { .buffer = s.instanceUboBuffer.frameBuffers[i], .offset = 0, .range = sizeof(math::InstanceUbo) };
							const VkWriteDescriptorSet write = {
								.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr,
								.dstSet = meshInstance.instanceDescriptorSets[i], .dstBinding = 0, .dstArrayElement = 0,
								.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								.pImageInfo = nullptr, .pBufferInfo = &bufferInfo, .pTexelBufferView = nullptr
							};
							vkUpdateDescriptorSets(s.logicalDevice, 1, &write, 0, nullptr);
						}
					}
				}));

		descriptorManager.define(DescriptorType::MATERIAL,
			VulkanDescriptor()
				.withLayout("Descriptor Set 2 - Material",
					{
						{ "Binding 0: Albedo",            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
						{ "Binding 1: Normal",            VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
						{ "Binding 2: Metallic",          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
						{ "Binding 3: Roughness",         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
						{ "Binding 4: Ambient Occlusion", VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
					})
				.withPoolSizes(
					{{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = static_cast<uint32_t>(MAX_MESHES) * parus::NUMBER_OF_TEXTURE_TYPES + IMAGE_SAMPLER_POOL }},
					static_cast<uint32_t>(MAX_MESHES) * parus::NUMBER_OF_TEXTURE_TYPES + IMAGE_SAMPLER_POOL)
				.withAllocator([&](VulkanStorage& s, const VkDescriptorSetLayout layout)
				{
					for (const auto& mesh : Services::get<World>()->getStorage()->getAllMeshes())
					{
						for (auto& meshPart : mesh->meshParts)
						{
							ASSERT(meshPart.material, "Material must exist for any mesh part.");
							auto* vulkanMaterial = dynamic_cast<vulkan::VulkanMaterial*>(meshPart.material.get());
							ASSERT(vulkanMaterial, "Expected vulkan::Material in Vulkan renderer path.");

							if (vulkanMaterial->materialDescriptorSet != VK_NULL_HANDLE)
							{
								continue;
							}

							VkDescriptorSetAllocateInfo allocInfo{};
							allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
							allocInfo.descriptorPool = s.descriptorPool;
							allocInfo.descriptorSetCount = 1;
							allocInfo.pSetLayouts = &layout;

							ASSERT(vkAllocateDescriptorSets(s.logicalDevice, &allocInfo, &vulkanMaterial->materialDescriptorSet) == VK_SUCCESS,
								"Failed to allocate material descriptor sets.");

							std::vector<VkDescriptorImageInfo> imageInfos;
							imageInfos.reserve(parus::NUMBER_OF_TEXTURE_TYPES);
							vulkanMaterial->iterateAllTextures([&]([[maybe_unused]] const parus::TextureType textureType, const std::shared_ptr<const VulkanTexture2d>& texture)
							{
								imageInfos.push_back({ .sampler = texture->sampler, .imageView = texture->imageView, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
							});

							std::vector<VkWriteDescriptorSet> writes;
							writes.reserve(imageInfos.size());
							for (uint32_t i = 0; i < imageInfos.size(); ++i)
							{
								writes.push_back({
									.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr,
									.dstSet = vulkanMaterial->materialDescriptorSet, .dstBinding = i, .dstArrayElement = 0,
									.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
									.pImageInfo = &imageInfos[i], .pBufferInfo = nullptr, .pTexelBufferView = nullptr
								});
							}
							vkUpdateDescriptorSets(s.logicalDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
						}
					}
				}));

		descriptorManager.define(DescriptorType::LIGHTS,
			VulkanDescriptor()
				.withLayout("Descriptor Set 3 - Lights",
					{
						{ "Binding 0: Light UBO",    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT },
						{ "Binding 1: Cube map",     VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
						{ "Binding 2: Shadow map",   VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
						{ "Binding 3: Point lights", VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         VK_SHADER_STAGE_FRAGMENT_BIT },
						{ "Binding 4: SSAO",         VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT },
					})
				.withPoolSizes(
					{
						{ .type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         .descriptorCount = VulkanStorage::MAX_FRAMES_IN_FLIGHT * 2 },
						{ .type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, .descriptorCount = VulkanStorage::MAX_FRAMES_IN_FLIGHT * 3 },
					},
					VulkanStorage::MAX_FRAMES_IN_FLIGHT)
				.withAllocator([&](VulkanStorage& s, const VkDescriptorSetLayout layout)
				{
					if (!directionalLight.descriptorSets.empty())
					{
						return;
					}

					const std::vector lightLayouts(VulkanStorage::MAX_FRAMES_IN_FLIGHT, layout);
					VkDescriptorSetAllocateInfo allocInfo{};
					allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
					allocInfo.descriptorPool = s.descriptorPool;
					allocInfo.descriptorSetCount = static_cast<uint32_t>(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
					allocInfo.pSetLayouts = lightLayouts.data();

					directionalLight.descriptorSets.resize(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
					ASSERT(vkAllocateDescriptorSets(s.logicalDevice, &allocInfo, directionalLight.descriptorSets.data()) == VK_SUCCESS,
						"Failed to allocate lights descriptor sets.");

					for (size_t i = 0; i < VulkanStorage::MAX_FRAMES_IN_FLIGHT; i++)
					{
						const VkDescriptorBufferInfo lightBufferInfo = { .buffer = s.directionalLightUboBuffer.frameBuffers[i], .offset = 0, .range = sizeof(math::DirectionalLightUbo) };
						const VkDescriptorImageInfo cubemapInfo = { .sampler = cubemap.sampler, .imageView = cubemap.imageView, .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
						const VkDescriptorImageInfo shadowMapInfo = { .sampler = shadowPass.getSampler(), .imageView = shadowPass.getImageView(), .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL };
						const VkDescriptorBufferInfo pointLightBufferInfo = { .buffer = s.pointLightUboBuffer.frameBuffers[i], .offset = 0, .range = sizeof(math::PointLightUbo) };
						const VkDescriptorImageInfo ssaoInfo = { .sampler = ssaoBlurPass.getSampler(), .imageView = ssaoBlurPass.getImageView(), .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

						const std::array<VkWriteDescriptorSet, 5> writes = {{
							{
								.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr,
								.dstSet = directionalLight.descriptorSets[i], .dstBinding = 0, .dstArrayElement = 0,
								.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								.pImageInfo = nullptr, .pBufferInfo = &lightBufferInfo, .pTexelBufferView = nullptr
							},
							{
								.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr,
								.dstSet = directionalLight.descriptorSets[i], .dstBinding = 1, .dstArrayElement = 0,
								.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
								.pImageInfo = &cubemapInfo, .pBufferInfo = nullptr, .pTexelBufferView = nullptr
							},
							{
								.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr,
								.dstSet = directionalLight.descriptorSets[i], .dstBinding = 2, .dstArrayElement = 0,
								.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
								.pImageInfo = &shadowMapInfo, .pBufferInfo = nullptr, .pTexelBufferView = nullptr
							},
							{
								.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr,
								.dstSet = directionalLight.descriptorSets[i], .dstBinding = 3, .dstArrayElement = 0,
								.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
								.pImageInfo = nullptr, .pBufferInfo = &pointLightBufferInfo, .pTexelBufferView = nullptr
							},
							{
								.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, .pNext = nullptr,
								.dstSet = directionalLight.descriptorSets[i], .dstBinding = 4, .dstArrayElement = 0,
								.descriptorCount = 1, .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
								.pImageInfo = &ssaoInfo, .pBufferInfo = nullptr, .pTexelBufferView = nullptr
							}
						}};
						vkUpdateDescriptorSets(s.logicalDevice, static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
					}
				}));
	}	

	void VulkanRenderer::clean()
	{
		mainPass.cleanup(storage);
		ssaoBlurPass.cleanup(storage);
		ssaoPass.cleanup(storage);
		depthPrePass.cleanup(storage);
		shadowPass.cleanup(storage);
		initializer.cleanup(storage, descriptorManager);
	}

	void VulkanRenderer::drawFrame()
	{
		if (!isRunning)
		{
			return;
		}

		currentFrame = (currentFrame + 1) % VulkanStorage::MAX_FRAMES_IN_FLIGHT;

		cleanupFrameResources();

		vkWaitForFences(storage.logicalDevice, 1, &storage.inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		const std::optional<uint32_t> imageIndex = acquireNextImage();
		if (!imageIndex.has_value())
		{
			return;
		}

		updateUniformBuffer(currentFrame);
		processLoadedMeshes();
		vkResetFences(storage.logicalDevice, 1, &storage.inFlightFences[currentFrame]);

		const auto commandBuffer = getCommandBuffer(currentFrame);

		resetCommandBuffer(currentFrame);
		recordCommandBuffer(commandBuffer, imageIndex.value());

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		const VkSemaphore waitSemaphores[] = { storage.imageAvailableSemaphores[currentFrame] };
		constexpr VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &storage.commandBuffers[currentFrame];

		const VkSemaphore signalSemaphores[] = { storage.renderFinishedSemaphores[currentFrame] };
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		ASSERT(utils::threadSafeQueueSubmit(storage, &submitInfo, storage.inFlightFences[currentFrame]) == VK_SUCCESS, "failed to submit draw command buffer.");

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
			initializer.recreateSwapChain(storage);
			depthPrePass.onSwapchainRecreate(storage, configurator);
			depthPrePass.initPipeline(storage, descriptorManager);
			ssaoPass.onSwapchainRecreate(storage, configurator);
			ssaoPass.initPipeline(storage, descriptorManager, depthPrePass);
			ssaoBlurPass.onSwapchainRecreate(storage, configurator);
			ssaoBlurPass.initPipeline(storage, depthPrePass, ssaoPass);

			// Force LIGHTS descriptor re-creation since SSAO blur texture was recreated
			directionalLight.descriptorSets.clear();
			rebuildDescriptorSets();
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
		if (!flushMeshQueue())
		{
			return;
		}
		rebuildSceneBuffers();
		rebuildDescriptorSets();

		if (needsCubemapCapture && storage.globalBuffers.skyVertexBuffer != VK_NULL_HANDLE)
		{
			captureSkyToCubemap();
			needsCubemapCapture = false;
		}
	}

	bool VulkanRenderer::flushMeshQueue()
	{
		std::vector<std::pair<std::string, std::shared_ptr<Mesh>>> pendingMeshes;

		{
			std::scoped_lock lock(importModelMutex);
			if (modelQueue.empty())
			{
				return false;
			}

			while (!modelQueue.empty())
			{
				pendingMeshes.push_back(modelQueue.front());
				modelQueue.pop();
			}
		}

		for (auto& [meshPath, newMesh] : pendingMeshes)
		{
			Services::get<World>()->getStorage()->addNewMesh(meshPath, newMesh);
			meshInstances.push_back({
				.mesh = newMesh,
				.transform = math::Matrix4x4::identity(),
				.instanceDescriptorSets = {}
			});
		}

		return true;
	}

	void VulkanRenderer::rebuildSceneBuffers()
	{
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

		if (!allVertices.empty()) { createVertexBuffer(allVertices); }
		if (!allIndices.empty())  { createIndexBuffer(allIndices); }

		storage.globalBuffers.totalVertices = allVertices.size();
		storage.globalBuffers.totalIndices  = allIndices.size();

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

		if (!allSkyVertices.empty()) { createSkyVertexBuffer(allSkyVertices); }
		if (!allSkyIndices.empty())  { createSkyIndexBuffer(allSkyIndices); }

		storage.globalBuffers.totalSkyVertices = allSkyVertices.size();
		storage.globalBuffers.totalSkyIndices  = allSkyIndices.size();
	}

	void VulkanRenderer::rebuildDescriptorSets()
	{
		descriptorManager.rebuildAll(storage);
	}

	void VulkanRenderer::loadSceneAssets()
	{
		directionalLight =
			{
				.light = {
					.color = math::Vector3(1.0f, 0.95f, 0.85f).trivial(),
					.direction = math::Vector3(66.0f, 70.0f, 429.0f).trivial()
					// .direction = math::Vector3(1.0f, 0.4f, 0.3f).trivial()

				},
				.descriptorSets = {}
			};

		skyHorizonColor = math::Vector3(0.85f, 0.92f, 1.0f);
		skyZenithColor  = math::Vector3(0.25f, 0.45f, 0.85f);

		// Point lights (e.g., torch inside the tunnel)
		pointLights.push_back({
			.position = math::Vector3(62.0f, 57.0f, -33.0f),
			.color = math::Vector3(1.0f, 0.7f, 0.3f),
			.radius = 80.0f,
			.intensity = 4.0f
		});

		createCubemapTexture();

		// Load sky mesh
		importMesh("bin/assets/skybox/dynamic_skybox.obj", MeshType::SKY);

		RUN_ASYNC(importMesh("bin/assets/terrain/floor.obj"););
		RUN_ASYNC(importMesh("bin/assets/indoor/indoor.obj"););
		RUN_ASYNC(importMesh("bin/assets/indoor/threshold.obj"););
		RUN_ASYNC(importMesh("bin/assets/indoor/torch.obj"););
	}

	std::optional<uint32_t> VulkanRenderer::acquireNextImage()
	{
		ASSERT(static_cast<size_t>(currentFrame) < storage.imageAvailableSemaphores.size() && currentFrame >= 0,
			"Current frame number is larger than the number of fences.");

		uint32_t imageIndex;
		const VkResult result = vkAcquireNextImageKHR(storage.logicalDevice, storage.swapChain, UINT64_MAX, storage.imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		if (result == VK_ERROR_OUT_OF_DATE_KHR)
		{
			initializer.recreateSwapChain(storage);
			depthPrePass.onSwapchainRecreate(storage, configurator);
			depthPrePass.initPipeline(storage, descriptorManager);
			ssaoPass.onSwapchainRecreate(storage, configurator);
			ssaoPass.initPipeline(storage, descriptorManager, depthPrePass);
			ssaoBlurPass.onSwapchainRecreate(storage, configurator);
			ssaoBlurPass.initPipeline(storage, depthPrePass, ssaoPass);
			directionalLight.descriptorSets.clear();
			rebuildDescriptorSets();
			return std::nullopt;
		}

		ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "failed to acquire swap chain image.");

		return imageIndex;
	}

	void VulkanRenderer::createCubemapTexture()
	{
		static constexpr uint32_t CUBE_FACE_COUNT = 6;
		const uint32_t faceSize = configurator.cubemapFaceSize;
		static constexpr VkFormat CUBEMAP_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

		cubemap.image = VkImageBuilder("Cubemap Image")
			.setWidth(faceSize)
			.setHeight(faceSize)
			.setArrayLayers(CUBE_FACE_COUNT)
			.setFormat(CUBEMAP_FORMAT)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
			.setFlags(VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT)
			.build(storage);

		cubemap.imageMemory = VkDeviceMemoryBuilder("Cubemap Image Memory")
			.setImage(cubemap.image)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		transitionImageLayout(cubemap.image, CUBEMAP_FORMAT,
			VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1, CUBE_FACE_COUNT);

		transitionImageLayout(cubemap.image, CUBEMAP_FORMAT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			1, CUBE_FACE_COUNT);

		cubemap.imageView = VkImageViewBuilder()
			.setImage(cubemap.image)
			.setViewType(VK_IMAGE_VIEW_TYPE_CUBE)
			.setFormat(CUBEMAP_FORMAT)
			.setComponents({VK_COMPONENT_SWIZZLE_R, VK_COMPONENT_SWIZZLE_G, VK_COMPONENT_SWIZZLE_B, VK_COMPONENT_SWIZZLE_A})
			.setAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.setLayerCount(CUBE_FACE_COUNT)
			.build("Cubemap Image View", storage);

		cubemap.sampler = VkSamplerBuilder("Cubemap Sampler")
			.setSamplerMode(VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE)
			.setBorderColor(VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE)
			.build(storage);

		Services::get<World>()->getStorage()->setCubemapTexture(std::make_shared<VulkanTexture2d>(cubemap));
	}

	void VulkanRenderer::captureSkyToCubemap()
	{
		const uint32_t CAPTURE_SIZE = configurator.cubemapFaceSize;
		static constexpr uint32_t CUBE_FACE_COUNT = 6;
		static constexpr VkFormat CUBEMAP_FORMAT = VK_FORMAT_R8G8B8A8_UNORM;

		vkDeviceWaitIdle(storage.logicalDevice);

		// Offscreen render pass for capture
		VkRenderPass captureRenderPass;
		{
			const VkAttachmentDescription colorAttachment =
			{
				.flags = 0,
				.format = CUBEMAP_FORMAT,
				.samples = VK_SAMPLE_COUNT_1_BIT,
				.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
				.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
				.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
				.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
				.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
				.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
			};

			const VkAttachmentReference colorAttachmentRef = { .attachment = 0, .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

			VkSubpassDescription subpass{};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorAttachmentRef;

			const VkSubpassDependency dependency =
			{
				.srcSubpass = VK_SUBPASS_EXTERNAL,
				.dstSubpass = 0,
				.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				.srcAccessMask = 0,
				.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
				.dependencyFlags = 0,
			};

			VkRenderPassCreateInfo renderPassInfo{};
			renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassInfo.attachmentCount = 1;
			renderPassInfo.pAttachments = &colorAttachment;
			renderPassInfo.subpassCount = 1;
			renderPassInfo.pSubpasses = &subpass;
			renderPassInfo.dependencyCount = 1;
			renderPassInfo.pDependencies = &dependency;

			ASSERT(vkCreateRenderPass(storage.logicalDevice, &renderPassInfo, nullptr, &captureRenderPass) == VK_SUCCESS,
				"Failed to create cubemap capture render pass.");
		}

		// Sky capture pipeline (no MSAA, uses capture render pass)
		VkPipeline capturePipeline;
		VkPipelineLayout capturePipelineLayout;
		{
			using DescriptorType = VulkanDescriptorManager::DescriptorType;
			VkPipelineBuilder("Sky Capture Pipeline")
				.setRenderPass(captureRenderPass)
				.useLayouts({ descriptorManager.getLayout(DescriptorType::GLOBAL) })
				.addStage(storage, VK_SHADER_STAGE_VERTEX_BIT,   "bin/shaders/sky.vert.spv")
				.addStage(storage, VK_SHADER_STAGE_FRAGMENT_BIT, "bin/shaders/sky.frag.spv")
				.withVertexInput(
					{{ .binding = 0, .stride = sizeof(math::Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX }},
					{{ .location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(math::Vertex, position) }})
				.withInputAssembly()
				.withViewportState()
				.withRasterization()
				.withMultisample(VK_SAMPLE_COUNT_1_BIT)
				.withDepthStencil(false)
				.withColorBlend()
				.withDynamicState()
				.build(storage, capturePipelineLayout, capturePipeline);
		}

		// Per-face 2D image views and framebuffers
		std::array<VkImageView,   CUBE_FACE_COUNT> faceViews{};
		std::array<VkFramebuffer, CUBE_FACE_COUNT> faceFramebuffers{};

		for (uint32_t faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
		{
			VkImageViewCreateInfo viewInfo{};
			viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			viewInfo.image = cubemap.image;
			viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			viewInfo.format = CUBEMAP_FORMAT;
			viewInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, faceIndex, 1 };

			ASSERT(vkCreateImageView(storage.logicalDevice, &viewInfo, nullptr, &faceViews[faceIndex]) == VK_SUCCESS,
				"Failed to create cubemap face image view.");

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = captureRenderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &faceViews[faceIndex];
			framebufferInfo.width  = CAPTURE_SIZE;
			framebufferInfo.height = CAPTURE_SIZE;
			framebufferInfo.layers = 1;

			ASSERT(vkCreateFramebuffer(storage.logicalDevice, &framebufferInfo, nullptr, &faceFramebuffers[faceIndex]) == VK_SUCCESS,
				"Failed to create cubemap face framebuffer.");
		}

		// View directions for each cubemap face (forward, up)
		struct FaceDirection { math::Vector3 forward; math::Vector3 up; };
		const std::array<FaceDirection, CUBE_FACE_COUNT> faceDirections = {{
			{{ 1,  0,  0}, {0, -1,  0}},  // +X
			{{-1,  0,  0}, {0, -1,  0}},  // -X
			{{ 0,  1,  0}, {0,  0,  1}},  // +Y
			{{ 0, -1,  0}, {0,  0, -1}},  // -Y
			{{ 0,  0,  1}, {0, -1,  0}},  // +Z
			{{ 0,  0, -1}, {0, -1,  0}},  // -Z
		}};

		const math::Matrix4x4 captureProjection = math::Matrix4x4::perspective(
			math::radians(90.0f), 1.0f, configurator.zNear, configurator.zFar);

		const std::shared_ptr<Mesh> skyMesh = Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::SKY)[0];
		const MeshPart& skyMeshPart = skyMesh->meshParts[0];

		// Render each face
		for (uint32_t faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
		{
			// Update global UBO with this face's view matrix
			math::GlobalUbo captureUbo{};
			captureUbo.view = math::Matrix4x4::lookAt(
				math::Vector3(0, 0, 0),
				faceDirections[faceIndex].forward,
				faceDirections[faceIndex].up).trivial();
			captureUbo.projection   = captureProjection.trivial();
			captureUbo.skyHorizonColor = skyHorizonColor.trivial();
			captureUbo.skyZenithColor  = skyZenithColor.trivial();
			memcpy(storage.globalUboBuffer.mapped[0], &captureUbo, sizeof(captureUbo));

			const VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(storage, utils::getCommandPool(storage));

			VkClearValue clearValue{};
			clearValue.color = {{ 0.0f, 0.0f, 0.0f, 1.0f }};

			VkRenderPassBeginInfo renderPassBeginInfo{};
			renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			renderPassBeginInfo.renderPass = captureRenderPass;
			renderPassBeginInfo.framebuffer = faceFramebuffers[faceIndex];
			renderPassBeginInfo.renderArea = { {0, 0}, {CAPTURE_SIZE, CAPTURE_SIZE} };
			renderPassBeginInfo.clearValueCount = 1;
			renderPassBeginInfo.pClearValues = &clearValue;

			vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

			vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, capturePipeline);

			VkViewport viewport{ 0, 0, static_cast<float>(CAPTURE_SIZE), static_cast<float>(CAPTURE_SIZE), 0.0f, 1.0f };
			vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

			VkRect2D scissor{ {0, 0}, {CAPTURE_SIZE, CAPTURE_SIZE} };
			vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

			const VkBuffer vertexBuffers[] = { storage.globalBuffers.skyVertexBuffer };
			constexpr VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffer, storage.globalBuffers.skyIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
				capturePipelineLayout, 0, 1,
				&storage.globalDescriptorSets[0], 0, nullptr);

			vkCmdDrawIndexed(commandBuffer,
				static_cast<uint32_t>(skyMeshPart.indexCount),
				1,
				static_cast<uint32_t>(skyMeshPart.indexOffset),
				static_cast<int32_t>(skyMeshPart.vertexOffset),
				0);

			vkCmdEndRenderPass(commandBuffer);

			utils::endSingleTimeCommands(storage, utils::getCommandPool(storage), commandBuffer);
		}

#ifdef SAVE_CAPTURE_CUBEMAP_TEXTURES
		// Transition all 6 faces COLOR_ATTACHMENT_OPTIMAL -> TRANSFER_SRC_OPTIMAL for readback
		{
			const VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(storage, utils::getCommandPool(storage));

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = cubemap.image;
			barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, CUBE_FACE_COUNT };
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0, nullptr, 0, nullptr, 1, &barrier);

			utils::endSingleTimeCommands(storage, utils::getCommandPool(storage), commandBuffer);
		}

		// Read back all 6 faces and save to bin/assets/cubemap/
		{
			static constexpr uint32_t NUMBER_OF_CHANNELS = 4;
			static constexpr const char* FACE_NAMES[CUBE_FACE_COUNT] = { "px", "nx", "py", "ny", "pz", "nz" };

			std::filesystem::create_directories("bin/assets/cubemap");

			const VkDeviceSize faceDataSize = static_cast<VkDeviceSize>(CAPTURE_SIZE) * CAPTURE_SIZE * NUMBER_OF_CHANNELS;
			const VkDeviceSize totalSize = faceDataSize * CUBE_FACE_COUNT;

			auto [readbackBuffer, readbackMemory] = VkBufferBuilder("Cubemap Readback Buffer")
				.setSize(totalSize)
				.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT)
				.setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
				.build(storage);

			{
				const VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(storage, utils::getCommandPool(storage));

				std::array<VkBufferImageCopy, CUBE_FACE_COUNT> copyRegions{};
				for (uint32_t faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
				{
					copyRegions[faceIndex].bufferOffset      = faceIndex * faceDataSize;
					copyRegions[faceIndex].bufferRowLength   = 0;
					copyRegions[faceIndex].bufferImageHeight = 0;
					copyRegions[faceIndex].imageSubresource  = { VK_IMAGE_ASPECT_COLOR_BIT, 0, faceIndex, 1 };
					copyRegions[faceIndex].imageOffset       = { 0, 0, 0 };
					copyRegions[faceIndex].imageExtent       = { CAPTURE_SIZE, CAPTURE_SIZE, 1 };
				}

				vkCmdCopyImageToBuffer(commandBuffer, cubemap.image,
					VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
					readbackBuffer,
					static_cast<uint32_t>(copyRegions.size()), copyRegions.data());

				utils::endSingleTimeCommands(storage, utils::getCommandPool(storage), commandBuffer);
			}

			void* mappedData;
			vkMapMemory(storage.logicalDevice, readbackMemory, 0, totalSize, 0, &mappedData);

			for (uint32_t faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
			{
				const uint8_t* facePixels = static_cast<const uint8_t*>(mappedData) + faceIndex * faceDataSize;
				const std::string filePath = std::string("bin/assets/cubemap/") + FACE_NAMES[faceIndex] + ".png";
				stbi_write_png(filePath.c_str(), static_cast<int>(CAPTURE_SIZE), static_cast<int>(CAPTURE_SIZE), NUMBER_OF_CHANNELS, facePixels, static_cast<int>(CAPTURE_SIZE * NUMBER_OF_CHANNELS));
				LOG_INFO("Saved cubemap face: " + filePath);
			}

			vkUnmapMemory(storage.logicalDevice, readbackMemory);
			vkDestroyBuffer(storage.logicalDevice, readbackBuffer, nullptr);
			vkFreeMemory(storage.logicalDevice,   readbackMemory,  nullptr);
		}

		// Transition all 6 faces TRANSFER_SRC_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
		{
			const VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(storage, utils::getCommandPool(storage));

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = cubemap.image;
			barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, CUBE_FACE_COUNT };
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0, 0, nullptr, 0, nullptr, 1, &barrier);

			utils::endSingleTimeCommands(storage, utils::getCommandPool(storage), commandBuffer);
		}
#else
		// Transition all 6 faces COLOR_ATTACHMENT_OPTIMAL -> SHADER_READ_ONLY_OPTIMAL
		{
			const VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(storage, utils::getCommandPool(storage));

			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = cubemap.image;
			barrier.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, CUBE_FACE_COUNT };
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
				0, 0, nullptr, 0, nullptr, 1, &barrier);

			utils::endSingleTimeCommands(storage, utils::getCommandPool(storage), commandBuffer);
		}
#endif // SAVE_CAPTURE_CUBEMAP_TEXTURES

		// Cleanup temporary capture resources
		for (uint32_t faceIndex = 0; faceIndex < CUBE_FACE_COUNT; faceIndex++)
		{
			vkDestroyFramebuffer(storage.logicalDevice, faceFramebuffers[faceIndex], nullptr);
			vkDestroyImageView(storage.logicalDevice,   faceViews[faceIndex],        nullptr);
		}
		vkDestroyPipeline(storage.logicalDevice,       capturePipeline,       nullptr);
		vkDestroyPipelineLayout(storage.logicalDevice, capturePipelineLayout, nullptr);
		vkDestroyRenderPass(storage.logicalDevice,     captureRenderPass,     nullptr);

		LOG_INFO("Sky captured to cubemap.");
	}
	
	void VulkanRenderer::resetCommandBuffer(const int bufferId) const
	{
		ASSERT(static_cast<size_t>(bufferId) < storage.commandBuffers.size() && bufferId >= 0, "current frame number is larger than number of fences.");

		ASSERT(vkResetCommandBuffer(storage.commandBuffers[bufferId], 0) == VK_SUCCESS, "Failed to reset command buffer.");
	}
	
	void VulkanRenderer::recordCommandBuffer(const VkCommandBuffer commandBufferToRecord, const uint32_t imageIndex) const
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		ASSERT(vkBeginCommandBuffer(commandBufferToRecord, &beginInfo) == VK_SUCCESS,
			"Failed to begin recording command buffer.");

		const FrameContext frame{ commandBufferToRecord, static_cast<uint32_t>(currentFrame), imageIndex };
		const SceneData scene{ meshInstances, directionalLight, pointLights };

		shadowPass.record(frame, storage, scene);
		depthPrePass.record(frame, storage, scene);
		ssaoPass.record(frame, storage, scene);
		ssaoBlurPass.record(frame, storage, scene);
		mainPass.record(frame, storage, scene);

		ASSERT(vkEndCommandBuffer(commandBufferToRecord) == VK_SUCCESS, "Failed to end recording command buffer.");
	}
	
	VkCommandBuffer VulkanRenderer::getCommandBuffer(const int bufferId) const
	{
		ASSERT(static_cast<size_t>(bufferId) < storage.commandBuffers.size() && bufferId >= 0, "current frame number is larger than number of fences.");

		return storage.commandBuffers[bufferId];
	}
	
	void VulkanRenderer::generateMipmaps(const VulkanTexture2d& texture, const VkFormat imageFormat, const int32_t texWidth, const int32_t texHeight)
	{
		// Check if image format supports linear blitting
		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(storage.physicalDevice, imageFormat, &formatProperties);

		ASSERT(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT,
			"Texture image format does not support linear blitting.");

		const VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(storage, utils::getCommandPool(storage));

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

		utils::endSingleTimeCommands(storage, utils::getCommandPool(storage), commandBuffer);
	}

	void VulkanRenderer::transitionImageLayout(
		const VkImage image,
		[[maybe_unused]] VkFormat format,
		const VkImageLayout oldLayout,
		const VkImageLayout newLayout,
		const uint32_t mipLevels,
		const uint32_t layerCount)
	{
		const VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(storage, utils::getCommandPool(storage));

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
		barrier.subresourceRange.layerCount = layerCount;

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

		utils::endSingleTimeCommands(storage, utils::getCommandPool(storage), commandBuffer);
	}

	void VulkanRenderer::copyBufferToImage(const VkBuffer buffer, const VkImage image, const uint32_t width, const uint32_t height)
	{
		const VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(storage, utils::getCommandPool(storage));

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

		utils::endSingleTimeCommands(storage, utils::getCommandPool(storage), commandBuffer);
	}

	void VulkanRenderer::createVertexBuffer(const std::vector<math::Vertex>& vertices)
	{
		if (storage.globalBuffers.vertexBuffer != VK_NULL_HANDLE)
		{
			frames[currentFrame].buffersToDelete.emplace_back(
			   storage.globalBuffers.vertexBuffer,
			   storage.globalBuffers.vertexBufferMemory
			);
		}

		const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		auto [stagingBuffer, stagingBufferMemory] = VkBufferBuilder("Vertex Staging Buffer")
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			.build(storage);

		// Fill vertex buffer data.
		void* data;
		vkMapMemory(storage.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), bufferSize);
		vkUnmapMemory(storage.logicalDevice, stagingBufferMemory);

		std::tie(storage.globalBuffers.vertexBuffer, storage.globalBuffers.vertexBufferMemory) = VkBufferBuilder("Vertex Buffer")
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		copyBuffer(stagingBuffer, storage.globalBuffers.vertexBuffer, bufferSize);
		vkDestroyBuffer(storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, stagingBufferMemory, nullptr);
	}

	void VulkanRenderer::createSkyVertexBuffer(const std::vector<math::Vertex>& vertices)
	{
		if (storage.globalBuffers.skyVertexBuffer != VK_NULL_HANDLE)
		{
			frames[currentFrame].buffersToDelete.emplace_back(
			   storage.globalBuffers.skyVertexBuffer,
			   storage.globalBuffers.skyVertexBufferMemory
			);
		}

		const VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

		auto [stagingBuffer, stagingBufferMemory] = VkBufferBuilder("Sky Vertex Staging Buffer")
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			.build(storage);

		// Fill vertex buffer data.
		void* data;
		vkMapMemory(storage.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, vertices.data(), bufferSize);
		vkUnmapMemory(storage.logicalDevice, stagingBufferMemory);

		std::tie(storage.globalBuffers.skyVertexBuffer, storage.globalBuffers.skyVertexBufferMemory) = VkBufferBuilder("Sky Vertex Buffer")
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		copyBuffer(stagingBuffer, storage.globalBuffers.skyVertexBuffer, bufferSize);
		vkDestroyBuffer(storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, stagingBufferMemory, nullptr);
	}


	void VulkanRenderer::copyBuffer(const VkBuffer srcBuffer, const VkBuffer dstBuffer, const VkDeviceSize size)
	{
		const VkCommandBuffer commandBuffer = utils::beginSingleTimeCommands(storage, utils::getCommandPool(storage));

		VkBufferCopy copyRegion{};
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		utils::endSingleTimeCommands(storage, utils::getCommandPool(storage), commandBuffer);
	}

	void VulkanRenderer::createSkyIndexBuffer(const std::vector<uint32_t>& indices)
	{
		if (storage.globalBuffers.skyIndexBuffer != VK_NULL_HANDLE)
		{
			frames[currentFrame].buffersToDelete.emplace_back(
			   storage.globalBuffers.skyIndexBuffer,
			   storage.globalBuffers.skyIndexBufferMemory
			);
		}

		const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		auto [stagingBuffer, stagingBufferMemory] = VkBufferBuilder("Sky Index Staging Buffer")
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			.build(storage);

		void* data;
		vkMapMemory(storage.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), bufferSize);
		vkUnmapMemory(storage.logicalDevice, stagingBufferMemory);

		std::tie(storage.globalBuffers.skyIndexBuffer, storage.globalBuffers.skyIndexBufferMemory) = VkBufferBuilder("Sky Index Buffer")
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		copyBuffer(stagingBuffer, storage.globalBuffers.skyIndexBuffer, bufferSize);

		vkDestroyBuffer(storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, stagingBufferMemory, nullptr);
	}

	void VulkanRenderer::createIndexBuffer(const std::vector<uint32_t>& indices)
	{
		if (storage.globalBuffers.indexBuffer != VK_NULL_HANDLE)
		{
			frames[currentFrame].buffersToDelete.emplace_back(
			   storage.globalBuffers.indexBuffer,
			   storage.globalBuffers.indexBufferMemory
			);
		}

		const VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

		auto [stagingBuffer, stagingBufferMemory] = VkBufferBuilder("Index Staging Buffer")
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_SRC_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
			.build(storage);

		void* data;
		vkMapMemory(storage.logicalDevice, stagingBufferMemory, 0, bufferSize, 0, &data);
		memcpy(data, indices.data(), (size_t)bufferSize);
		vkUnmapMemory(storage.logicalDevice, stagingBufferMemory);

		std::tie(storage.globalBuffers.indexBuffer, storage.globalBuffers.indexBufferMemory) = VkBufferBuilder("Index Buffer")
			.setSize(bufferSize)
			.setUsage(VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
			.setMemoryProperties(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		copyBuffer(stagingBuffer, storage.globalBuffers.indexBuffer, bufferSize);

		vkDestroyBuffer(storage.logicalDevice, stagingBuffer, nullptr);
		vkFreeMemory(storage.logicalDevice, stagingBufferMemory, nullptr);
	}

	void VulkanRenderer::updateUniformBuffer(const uint32_t currentImage)
	{
		const SpectatorCamera camera = Services::get<World>()->getMainCamera();

		// Compute light-space matrix for shadow mapping
		const math::Vector3 lightDir = math::Vector3(
			directionalLight.light.direction.x,
			directionalLight.light.direction.y,
			directionalLight.light.direction.z).normalize();

		const float shadowExtent = configurator.shadowExtent;
		const float shadowNear = configurator.shadowNear;
		const float shadowFar = configurator.shadowFar;

		const math::Vector3 shadowCenter = camera.getPosition();
		const math::Vector3 lightPos = shadowCenter + lightDir * (shadowFar * 0.5f);

		const math::Matrix4x4 lightView = math::Matrix4x4::lookAt(
			lightPos,
			shadowCenter,
			math::Vector3(0.0f, 1.0f, 0.0f));

		const math::Matrix4x4 lightProj = math::Matrix4x4::orthographic(
			-shadowExtent, shadowExtent,
			-shadowExtent, shadowExtent,
			shadowNear, shadowFar);

		const math::Matrix4x4 lightSpaceMatrix = lightView * lightProj;

		// Shadow texel snapping - eliminates shadow shimmer on camera movement
		math::TrivialMatrix4x4 snappedLightSpaceMatrix = lightSpaceMatrix.trivial();
		{
			const float shadowMapSize = static_cast<float>(configurator.shadowMapSize);
			const float texelSize = (2.0f * shadowExtent) / shadowMapSize;

			snappedLightSpaceMatrix.values[3][0] = std::floor(snappedLightSpaceMatrix.values[3][0] / texelSize) * texelSize;
			snappedLightSpaceMatrix.values[3][1] = std::floor(snappedLightSpaceMatrix.values[3][1] / texelSize) * texelSize;
		}

		// Global UBO
		math::GlobalUbo globalUbo{};

		globalUbo.view = math::Matrix4x4::lookAt(
			camera.getPosition(),
			camera.getPosition() + camera.getForwardVector(),
			camera.getUpVector()).trivial();

		globalUbo.projection = math::Matrix4x4::perspective(
			math::radians(configurator.fieldOfView),
			static_cast<float>(storage.swapChainDetails.swapChainExtent.width) / static_cast<float>(storage.swapChainDetails.swapChainExtent.height),
			configurator.zNear, configurator.zFar).trivial();

		globalUbo.lightSpaceMatrix = snappedLightSpaceMatrix;
		globalUbo.cameraPosition = camera.getPosition().trivial();

		globalUbo.debug = debugMode;
		globalUbo.skyHorizonColor = skyHorizonColor.trivial();
		globalUbo.skyZenithColor = skyZenithColor.trivial();
		globalUbo.fogStart = configurator.fogStart;
		globalUbo.fogEnd = configurator.fogEnd;

		// Time for animated effects (sky clouds, etc.)
		static const auto startTime = std::chrono::high_resolution_clock::now();
		const auto currentTime = std::chrono::high_resolution_clock::now();
		globalUbo.time = std::chrono::duration<float>(currentTime - startTime).count();

		// Sun direction (normalized light direction for sky shader)
		globalUbo.sunDirection = lightDir.trivial();

		memcpy(storage.globalUboBuffer.mapped[currentImage], &globalUbo, sizeof(globalUbo));

		// Instance UBO
		math::InstanceUbo instanceUbo{};
		instanceUbo.model = math::Matrix4x4().trivial();
		instanceUbo.normal = math::Matrix4x4().trivial();

		memcpy(storage.instanceUboBuffer.mapped[currentImage], &instanceUbo, sizeof(instanceUbo));

		// Directional Light UBO
		math::DirectionalLightUbo directionalLightUbo{};
		directionalLightUbo.color = directionalLight.light.color;
		directionalLightUbo.direction = directionalLight.light.direction;

		memcpy(storage.directionalLightUboBuffer.mapped[currentFrame], &directionalLightUbo, sizeof(directionalLightUbo));

		// Point Light UBO
		math::PointLightUbo pointLightUbo{};
		pointLightUbo.count = static_cast<int>(std::min(pointLights.size(), static_cast<size_t>(math::MAX_POINT_LIGHTS)));
		for (int i = 0; i < pointLightUbo.count; ++i)
		{
			pointLightUbo.lights[i].posX = pointLights[i].position.x;
			pointLightUbo.lights[i].posY = pointLights[i].position.y;
			pointLightUbo.lights[i].posZ = pointLights[i].position.z;
			pointLightUbo.lights[i].radius = pointLights[i].radius;
			pointLightUbo.lights[i].colorR = pointLights[i].color.x;
			pointLightUbo.lights[i].colorG = pointLights[i].color.y;
			pointLightUbo.lights[i].colorB = pointLights[i].color.z;
			pointLightUbo.lights[i].intensity = pointLights[i].intensity;
		}

		memcpy(storage.pointLightUboBuffer.mapped[currentFrame], &pointLightUbo, sizeof(pointLightUbo));
	}

	void VulkanRenderer::onResize()
	{
		framebufferResized = true;
	}

	void VulkanRenderer::cleanupFrameResources()
	{
		// Destroy buffers scheduled for deletion from N frames ago
		const uint32_t frameToClean = (currentFrame + 1) % VulkanStorage::MAX_FRAMES_IN_FLIGHT;

		for (auto& [buffer, memory] : frames[frameToClean].buffersToDelete)
		{
			vkDestroyBuffer(storage.logicalDevice, buffer, nullptr);
			vkFreeMemory(storage.logicalDevice, memory, nullptr);
		}
		frames[frameToClean].buffersToDelete.clear();
	}

}