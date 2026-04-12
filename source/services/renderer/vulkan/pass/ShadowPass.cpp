#include "ShadowPass.h"

#include <array>

#include "services/renderer/vulkan/storage/VulkanStorage.h"
#include "services/renderer/vulkan/VulkanConfigurator.h"
#include "services/renderer/vulkan/VulkanDescriptorManager.h"
#include "services/renderer/vulkan/builder/VkImageBuilder.h"
#include "services/renderer/vulkan/builder/VkImageViewBuilder.h"
#include "services/renderer/vulkan/builder/VkDeviceMemoryBuilder.h"
#include "services/renderer/vulkan/builder/VkPipelineBuilder.h"
#include "services/renderer/vulkan/utils/VulkanUtils.h"
#include "services/renderer/vulkan/mesh/MeshInstance.h"
#include "services/renderer/vulkan/mesh/Mesh.h"
#include "engine/utils/math/Math.h"


namespace parus::vulkan
{

	void ShadowPass::init(VulkanStorage& storage, const VulkanConfigurator& config)
	{
		shadowMapSize = config.shadowMapSize;
		const VkFormat depthFormat = utils::findDepthFormat(storage);

		// Create shadow map depth image
		image = VkImageBuilder("Shadow Map Image")
			.setWidth(shadowMapSize)
			.setHeight(shadowMapSize)
			.setFormat(depthFormat)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.build(storage);

		imageMemory = VkDeviceMemoryBuilder("Shadow Map Memory")
			.setImage(image)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		imageView = VkImageViewBuilder()
			.setImage(image)
			.setViewType(VK_IMAGE_VIEW_TYPE_2D)
			.setFormat(depthFormat)
			.setAspectMask(VK_IMAGE_ASPECT_DEPTH_BIT)
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

		ASSERT(vkCreateSampler(storage.logicalDevice, &samplerInfo, nullptr, &sampler) == VK_SUCCESS,
			"Failed to create shadow map sampler.");
		utils::setDebugName(storage, sampler, VK_OBJECT_TYPE_SAMPLER, "Shadow Map Sampler");

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

		VkRenderPassCreateInfo renderPassCreateInfo{};
		renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassCreateInfo.attachmentCount = 1;
		renderPassCreateInfo.pAttachments = &depthAttachment;
		renderPassCreateInfo.subpassCount = 1;
		renderPassCreateInfo.pSubpasses = &subpass;
		renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
		renderPassCreateInfo.pDependencies = dependencies.data();

		ASSERT(vkCreateRenderPass(storage.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass) == VK_SUCCESS,
			"Failed to create shadow render pass.");
		utils::setDebugName(storage, renderPass, VK_OBJECT_TYPE_RENDER_PASS, "Shadow Map Render Pass");

		// Create shadow framebuffer
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = 1;
		framebufferInfo.pAttachments = &imageView;
		framebufferInfo.width = shadowMapSize;
		framebufferInfo.height = shadowMapSize;
		framebufferInfo.layers = 1;

		ASSERT(vkCreateFramebuffer(storage.logicalDevice, &framebufferInfo, nullptr, &framebuffer) == VK_SUCCESS,
			"Failed to create shadow map framebuffer.");
		utils::setDebugName(storage, framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "Shadow Map Framebuffer");
	}

	void ShadowPass::initPipeline(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager, const VulkanConfigurator& config)
	{
		using DescriptorType = VulkanDescriptorManager::DescriptorType;

		VkPipelineBuilder("Shadow Pipeline")
			.setRenderPass(renderPass)
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
			.withRasterization(VK_CULL_MODE_FRONT_BIT, true, config.shadowDepthBiasConstant, config.shadowDepthBiasSlope)
			.withMultisample(VK_SAMPLE_COUNT_1_BIT)
			.withDepthStencil(true)
			.withNoColorBlend()
			.withDynamicState()
			.build(storage, pipelineLayout, pipeline);
	}

	void ShadowPass::record(const FrameContext& frame, const VulkanStorage& storage, const SceneData& scene) const
	{
		if (!storage.globalBuffers.vertexBuffer)
		{
			return;
		}

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea = { {0, 0}, {shadowMapSize, shadowMapSize} };

		VkClearValue clearValue{};
		clearValue.depthStencil = { .depth = 1.0f, .stencil = 0 };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(frame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkViewport viewport{};
		viewport.width = static_cast<float>(shadowMapSize);
		viewport.height = static_cast<float>(shadowMapSize);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = { {0, 0}, {shadowMapSize, shadowMapSize} };
		vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);

		const VkBuffer vertexBuffers[] = { storage.globalBuffers.vertexBuffer };
		constexpr VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(frame.commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(frame.commandBuffer, storage.globalBuffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Bind global descriptor (contains lightSpaceMatrix)
		vkCmdBindDescriptorSets(
			frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&storage.globalDescriptorSets[frame.currentFrame], 0, nullptr);

		for (const auto& meshInstance : scene.meshInstances)
		{
			if (meshInstance.mesh->meshType != MeshType::STATIC_MESH)
			{
				continue;
			}

			// Bind instance descriptor
			vkCmdBindDescriptorSets(
				frame.commandBuffer,
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				pipelineLayout,
				1,
				1,
				&meshInstance.instanceDescriptorSets[frame.currentFrame], 0, nullptr);

			for (const auto& meshPart : meshInstance.mesh->meshParts)
			{
				vkCmdDrawIndexed(frame.commandBuffer,
					static_cast<uint32_t>(meshPart.indexCount),
					1,
					static_cast<uint32_t>(meshPart.indexOffset),
					static_cast<int32_t>(meshPart.vertexOffset),
					0);
			}
		}

		vkCmdEndRenderPass(frame.commandBuffer);
	}

	void ShadowPass::cleanup(const VulkanStorage& storage)
	{
		if (sampler != VK_NULL_HANDLE)
		{
			vkDestroySampler(storage.logicalDevice, sampler, nullptr);
			sampler = VK_NULL_HANDLE;
		}
		if (imageView != VK_NULL_HANDLE)
		{
			vkDestroyImageView(storage.logicalDevice, imageView, nullptr);
			imageView = VK_NULL_HANDLE;
		}
		if (image != VK_NULL_HANDLE)
		{
			vkDestroyImage(storage.logicalDevice, image, nullptr);
			image = VK_NULL_HANDLE;
		}
		if (imageMemory != VK_NULL_HANDLE)
		{
			vkFreeMemory(storage.logicalDevice, imageMemory, nullptr);
			imageMemory = VK_NULL_HANDLE;
		}

		VulkanRenderPass::cleanup(storage);
	}

}
