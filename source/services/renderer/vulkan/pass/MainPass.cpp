#include "MainPass.h"

#include <array>

#include "services/renderer/vulkan/storage/VulkanStorage.h"
#include "services/renderer/vulkan/VulkanConfigurator.h"
#include "services/renderer/vulkan/VulkanDescriptorManager.h"
#include "services/renderer/vulkan/builder/VkPipelineBuilder.h"
#include "services/renderer/vulkan/utils/VulkanUtils.h"
#include "services/renderer/vulkan/mesh/MeshInstance.h"
#include "services/renderer/vulkan/mesh/Mesh.h"
#include "services/renderer/vulkan/material/Material.h"
#include "engine/utils/math/Math.h"
#include "services/Services.h"
#include "services/graphics/imgui/ImGuiLibrary.h"
#include "services/world/World.h"


namespace parus::vulkan
{

	void MainPass::init(VulkanStorage& storage, const VulkanConfigurator& config)
	{
		// MainPass uses the shared main render pass and swapchain framebuffers
		// which are created by VulkanInitializer. Nothing to init here.
	}

	void MainPass::initPipelines(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager)
	{
		using DescriptorType = VulkanDescriptorManager::DescriptorType;

		// Sky pipeline
		VkPipelineBuilder("Sky Pipeline Layout")
			.useLayouts({descriptorManager.getLayout(DescriptorType::GLOBAL)})
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
			.build(storage, skyPipelineLayout, skyPipeline);

		// Main scene pipeline
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
			.build(storage, pipelineLayout, pipeline);
	}

	void MainPass::record(const FrameContext& frame, const VulkanStorage& storage, const SceneData& scene) const
	{
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = storage.renderPass;
		renderPassInfo.framebuffer = storage.swapChainFramebuffers[frame.imageIndex];
		renderPassInfo.renderArea.offset = { .x = 0, .y = 0 };
		renderPassInfo.renderArea.extent = storage.swapChainDetails.swapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
		clearValues[1].depthStencil = { .depth = 1.0f, .stencil = 0 };

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(frame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		drawSkyboxPass(frame, storage);
		drawMainScenePass(frame, storage, scene);
		Services::get<imgui::ImGuiLibrary>()->renderDrawData(frame.commandBuffer);
		vkCmdEndRenderPass(frame.commandBuffer);
	}

	void MainPass::cleanup(const VulkanStorage& storage)
	{
		if (skyPipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(storage.logicalDevice, skyPipeline, nullptr);
			skyPipeline = VK_NULL_HANDLE;
		}
		if (skyPipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(storage.logicalDevice, skyPipelineLayout, nullptr);
			skyPipelineLayout = VK_NULL_HANDLE;
		}

		VulkanRenderPass::cleanup(storage);
	}

	void MainPass::setFullscreenViewportScissor(const VkCommandBuffer commandBuffer, const VulkanStorage& storage) const
	{
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(storage.swapChainDetails.swapChainExtent.width);
		viewport.height = static_cast<float>(storage.swapChainDetails.swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		const VkRect2D scissor = { {0, 0}, storage.swapChainDetails.swapChainExtent };
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void MainPass::drawSkyboxPass(const FrameContext& frame, const VulkanStorage& storage) const
	{
		ASSERT(!Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::SKY).empty(),
			   "Sky mesh must always exist");

		const std::shared_ptr<Mesh> skyMesh = Services::get<World>()->getStorage()->getAllMeshesByType(MeshType::SKY)[0];

		ASSERT(!skyMesh->meshParts.empty(),
			   "Sky mesh exists, but it has zero mesh parts.");

		const MeshPart& skyMeshPart = skyMesh->meshParts[0];

		ASSERT(skyMeshPart.vertexCount > 0 && skyMeshPart.indexCount > 0,
			   "Sky mesh has no vertices or indices");

		vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, skyPipeline);
		setFullscreenViewportScissor(frame.commandBuffer, storage);

		const VkBuffer skyVertexBuffers[] = { storage.globalBuffers.skyVertexBuffer };
		constexpr VkDeviceSize skyOffsets[] = { 0 };
		vkCmdBindVertexBuffers(frame.commandBuffer, 0, 1, skyVertexBuffers, skyOffsets);
		vkCmdBindIndexBuffer(frame.commandBuffer, storage.globalBuffers.skyIndexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdBindDescriptorSets(
			frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			skyPipelineLayout,
			0, 1,
			&storage.globalDescriptorSets[frame.currentFrame],
			0, nullptr);

		vkCmdDrawIndexed(frame.commandBuffer,
			 static_cast<uint32_t>(skyMeshPart.indexCount),
			 1,
			 static_cast<uint32_t>(skyMeshPart.indexOffset),
			 static_cast<int32_t>(skyMeshPart.vertexOffset),
			 0);
	}

	void MainPass::drawMainScenePass(const FrameContext& frame, const VulkanStorage& storage, const SceneData& scene) const
	{
		if (!storage.globalBuffers.vertexBuffer)
		{
			return;
		}

		vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		setFullscreenViewportScissor(frame.commandBuffer, storage);

		const VkBuffer vertexBuffers[] = { storage.globalBuffers.vertexBuffer };
		constexpr VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(frame.commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(frame.commandBuffer, storage.globalBuffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Bind the global descriptor set.
		vkCmdBindDescriptorSets(
			frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&storage.globalDescriptorSets[frame.currentFrame], 0, nullptr);

		// Bind the directional light descriptor set.
		vkCmdBindDescriptorSets(
			frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			3,
			1,
			&scene.directionalLight.descriptorSets[frame.currentFrame], 0, nullptr);

		for (const auto& meshInstance : scene.meshInstances)
		{
			if (meshInstance.mesh->meshType != MeshType::STATIC_MESH)
			{
				continue;
			}

			// Bind the instance descriptor set.
			vkCmdBindDescriptorSets(
				frame.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1,
				1,
				&meshInstance.instanceDescriptorSets[frame.currentFrame], 0, nullptr);

			for (const auto& meshPart : meshInstance.mesh->meshParts)
			{
				// Bind the material descriptor set.
				vkCmdBindDescriptorSets(
					frame.commandBuffer,
					VK_PIPELINE_BIND_POINT_GRAPHICS,
					pipelineLayout,
					2,
					1,
					&meshPart.material->materialDescriptorSet, 0, nullptr);

				// Draw mesh part.
				vkCmdDrawIndexed(frame.commandBuffer,
					 static_cast<uint32_t>(meshPart.indexCount),
					 1,
					 static_cast<uint32_t>(meshPart.indexOffset),
					 static_cast<int32_t>(meshPart.vertexOffset),
					 0);
			}
		}
	}

}
