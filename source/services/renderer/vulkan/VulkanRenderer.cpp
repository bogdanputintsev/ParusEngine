#include "VulkanRenderer.h"

#include <array>
#include <cassert>
#include <set>
#include <stdexcept>

#include "builder/VkBufferBuilder.h"
#include "builder/VkDeviceMemoryBuilder.h"
#include "builder/VkImageBuilder.h"
#include "builder/VkImageViewBuilder.h"
#include "builder/VkSamplerBuilder.h"
#include "engine/input/Input.h"
#include "services/platform/Platform.h"
#include "engine/Event.h"
#include "engine/utils/Utils.h"
#include "services/graphics/imgui/ImGuiLibrary.h"
#include "engine/utils/math/UniformBufferObjects.h"
#include "material/Material.h"
#include "services/Services.h"
#include "services/threading/ThreadPool.h"
#include "services/world/World.h"
#include "utils/VulkanUtils.h"


namespace parus::vulkan
{
	void VulkanRenderer::init()
	{
		registerEvents();

		initializer.initialize(storage);

		loadSceneAssets();

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
		initializer.cleanup(storage);
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
	}

	bool VulkanRenderer::flushMeshQueue()
	{
		std::unique_lock lock(importModelMutex);
		if (modelQueue.empty())
		{
			return false;
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
		for (auto& mesh : Services::get<World>()->getStorage()->getAllMeshes())
		{
			createMeshDescriptorSets(mesh);
		}
		createLightsDescriptorSets();
	}

	void VulkanRenderer::loadSceneAssets()
	{
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
			return std::nullopt;
		}

		ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "failed to acquire swap chain image.");

		return imageIndex;
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

	void VulkanRenderer::setFullscreenViewportScissor(const VkCommandBuffer cmd) const
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(storage.swapChainDetails.swapChainExtent.width);
		viewport.height = static_cast<float>(storage.swapChainDetails.swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(cmd, 0, 1, &viewport);

		const VkRect2D scissor = { {0, 0}, storage.swapChainDetails.swapChainExtent };
		vkCmdSetScissor(cmd, 0, 1, &scissor);
	}

	void VulkanRenderer::resetCommandBuffer(const int bufferId) const
	{
		ASSERT(static_cast<size_t>(bufferId) < storage.commandBuffers.size() && bufferId >= 0, "current frame number is larger than number of fences.");

		ASSERT(vkResetCommandBuffer(storage.commandBuffers[bufferId], 0) == VK_SUCCESS, "Failed to reset command buffer.");
	}

	void VulkanRenderer::drawMainScenePass(const VkCommandBuffer commandBufferToRecord) const
	{
		if (!storage.globalBuffers.vertexBuffer)
		{
			return;
		}

		vkCmdBindPipeline(commandBufferToRecord, VK_PIPELINE_BIND_POINT_GRAPHICS, storage.mainPipeline);
		setFullscreenViewportScissor(commandBufferToRecord);

		const VkBuffer vertexBuffers[] = { storage.globalBuffers.vertexBuffer };
		constexpr VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBufferToRecord, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBufferToRecord, storage.globalBuffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Bind the global descriptor set.
		vkCmdBindDescriptorSets(
			commandBufferToRecord,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			storage.mainPipelineLayout,
			0,
			1,
			&storage.globalDescriptorSets[currentFrame], 0, nullptr);

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
		renderPassInfo.framebuffer = storage.swapChainFramebuffers[imageIndex];
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
		ASSERT(!Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::SKY).empty(),
			   "Sky mesh must always exist");

		const std::shared_ptr<Mesh> skyMesh = Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::SKY)[0];

		ASSERT(!skyMesh->meshParts.empty(),
			   "Sky mesh exists, but it has zero mesh parts.");

		const MeshPart& skyMeshPart = skyMesh->meshParts[0];

		ASSERT(skyMeshPart.vertexCount > 0 && skyMeshPart.indexCount > 0,
			   "Sky mesh has no vertices or indices");

		vkCmdBindPipeline(commandBufferToRecord, VK_PIPELINE_BIND_POINT_GRAPHICS, storage.skyPipeline);
		setFullscreenViewportScissor(commandBufferToRecord);

		// Sky vertices
		const VkBuffer skyVertexBuffers[] = { storage.globalBuffers.skyVertexBuffer };
		constexpr VkDeviceSize skyOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBufferToRecord, 0, 1, skyVertexBuffers, skyOffsets);
		vkCmdBindIndexBuffer(commandBufferToRecord, storage.globalBuffers.skyIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Bind
		vkCmdBindDescriptorSets(
			commandBufferToRecord,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			storage.skyPipelineLayout,
			0, 1,
			&storage.globalDescriptorSets[currentFrame],
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
		const uint32_t mipLevels)
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
	}

	void VulkanRenderer::createMeshDescriptorSets(const std::shared_ptr<Mesh>& mesh)
	{
		createGlobalDescriptorSets();
		createInstanceDescriptorSets();
		createMaterialDescriptorSets(mesh);
	}

	void VulkanRenderer::createGlobalDescriptorSets()
	{
		const std::vector globalLayouts(VulkanStorage::MAX_FRAMES_IN_FLIGHT, storage.globalDescriptorSetLayout);
		VkDescriptorSetAllocateInfo globalAllocateInfo{};
		globalAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		globalAllocateInfo.descriptorPool = storage.descriptorPool;
		globalAllocateInfo.descriptorSetCount = static_cast<uint32_t>(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
		globalAllocateInfo.pSetLayouts = globalLayouts.data();

		ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &globalAllocateInfo, storage.globalDescriptorSets.data()) == VK_SUCCESS,
			"Failed to allocate global descriptor sets.");

		for (size_t frameIndex = 0; frameIndex < VulkanStorage::MAX_FRAMES_IN_FLIGHT; frameIndex++)
		{
			// Global UBO descriptor set
			const VkDescriptorBufferInfo globalBufferInfo =
				{
					.buffer = storage.globalUboBuffer.frameBuffers[frameIndex],
					.offset = 0,
					.range = sizeof(math::GlobalUbo)
				};

			const VkWriteDescriptorSet globalWrite =
				{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = storage.globalDescriptorSets[frameIndex],
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
			const std::vector instanceLayouts(VulkanStorage::MAX_FRAMES_IN_FLIGHT, storage.instanceDescriptorSetLayout);
			VkDescriptorSetAllocateInfo instanceSetAllocateInfo{};
			instanceSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			instanceSetAllocateInfo.descriptorPool = storage.descriptorPool;
			instanceSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
			instanceSetAllocateInfo.pSetLayouts = instanceLayouts.data();

			meshInstance.instanceDescriptorSets.resize(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
			ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &instanceSetAllocateInfo, meshInstance.instanceDescriptorSets.data()) == VK_SUCCESS,
				"Failed to allocate instance descriptor sets.");

			for (size_t frameIndex = 0; frameIndex < VulkanStorage::MAX_FRAMES_IN_FLIGHT; frameIndex++)
			{
				// Instance UBO descriptor set
				const VkDescriptorBufferInfo instanceBufferInfo =
					{
						.buffer = storage.instanceUboBuffer.frameBuffers[frameIndex],
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

		const std::vector lightLayouts(VulkanStorage::MAX_FRAMES_IN_FLIGHT, storage.lightsDescriptorSetLayout);
		VkDescriptorSetAllocateInfo lightSetAllocateInfo{};
		lightSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		lightSetAllocateInfo.descriptorPool = storage.descriptorPool;
		lightSetAllocateInfo.descriptorSetCount = static_cast<uint32_t>(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
		lightSetAllocateInfo.pSetLayouts = lightLayouts.data();

		directionalLight.descriptorSets.resize(VulkanStorage::MAX_FRAMES_IN_FLIGHT);
		ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &lightSetAllocateInfo, directionalLight.descriptorSets.data()) == VK_SUCCESS,
			"Failed to allocate instance descriptor sets.");

		for (size_t frameIndex = 0; frameIndex < VulkanStorage::MAX_FRAMES_IN_FLIGHT; frameIndex++)
		{
			std::array<VkWriteDescriptorSet, 2> descriptorWrites;

			// Light UBO descriptor set
			const VkDescriptorBufferInfo directionalLightBufferInfo =
				{
					.buffer = storage.directionalLightUboBuffer.frameBuffers[frameIndex],
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