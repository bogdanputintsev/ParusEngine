#include "DepthPrePass.h"

#include <array>

#include "services/renderer/vulkan/storage/VulkanStorage.h"
#include "services/renderer/vulkan/VulkanConfigurator.h"
#include "services/renderer/vulkan/VulkanDescriptorManager.h"
#include "services/renderer/vulkan/builder/VkImageBuilder.h"
#include "services/renderer/vulkan/builder/VkImageViewBuilder.h"
#include "services/renderer/vulkan/builder/VkDeviceMemoryBuilder.h"
#include "services/renderer/vulkan/builder/VkSamplerBuilder.h"
#include "services/renderer/vulkan/builder/VkPipelineBuilder.h"
#include "services/renderer/vulkan/utils/VulkanUtils.h"
#include "services/renderer/vulkan/mesh/MeshInstance.h"
#include "engine/utils/math/Math.h"
#include "services/renderer/vulkan/mesh/Mesh.h"


namespace parus::vulkan
{

	void DepthPrePass::init(VulkanStorage& storage, const VulkanConfigurator& config)
	{
		const uint32_t screenWidth = storage.swapChainDetails.swapChainExtent.width;
		const uint32_t screenHeight = storage.swapChainDetails.swapChainExtent.height;
		const VkFormat depthFormat = utils::findDepthFormat(storage);

		image = VkImageBuilder("Depth Pre-Pass Image")
			.setWidth(screenWidth)
			.setHeight(screenHeight)
			.setFormat(depthFormat)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.build(storage);

		imageMemory = VkDeviceMemoryBuilder("Depth Pre-Pass Memory")
			.setImage(image)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		imageView = VkImageViewBuilder()
			.setImage(image)
			.setViewType(VK_IMAGE_VIEW_TYPE_2D)
			.setFormat(depthFormat)
			.setAspectMask(VK_IMAGE_ASPECT_DEPTH_BIT)
			.build("Depth Pre-Pass View", storage);

		sampler = VkSamplerBuilder("Depth Pre-Pass Sampler")
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

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = 1;
			renderPassCreateInfo.pAttachments = &depthAttachment;
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassCreateInfo.pDependencies = dependencies.data();

			ASSERT(vkCreateRenderPass(storage.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass) == VK_SUCCESS,
				"Failed to create depth pre-pass render pass.");
			utils::setDebugName(storage, renderPass, VK_OBJECT_TYPE_RENDER_PASS, "Depth Pre-Pass Render Pass");
		}

		// Depth pre-pass framebuffer
		{
			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = &imageView;
			framebufferInfo.width = screenWidth;
			framebufferInfo.height = screenHeight;
			framebufferInfo.layers = 1;

			ASSERT(vkCreateFramebuffer(storage.logicalDevice, &framebufferInfo, nullptr, &framebuffer) == VK_SUCCESS,
				"Failed to create depth pre-pass framebuffer.");
			utils::setDebugName(storage, framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "Depth Pre-Pass Framebuffer");
		}
	}

	void DepthPrePass::initPipeline(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager)
	{
		using DescriptorType = VulkanDescriptorManager::DescriptorType;

		VkPipelineBuilder("Depth Pre-Pass Pipeline")
			.setRenderPass(renderPass)
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
			.build(storage, pipelineLayout, pipeline);
	}

	void DepthPrePass::record(const FrameContext& frame, const VulkanStorage& storage, const SceneData& scene) const
	{
		if (!storage.globalBuffers.vertexBuffer)
		{
			return;
		}

		const VkExtent2D extent = storage.swapChainDetails.swapChainExtent;

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = framebuffer;
		renderPassInfo.renderArea = { {0, 0}, extent };

		VkClearValue clearValue{};
		clearValue.depthStencil = { .depth = 1.0f, .stencil = 0 };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearValue;

		vkCmdBeginRenderPass(frame.commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(frame.commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

		VkViewport viewport{};
		viewport.width = static_cast<float>(extent.width);
		viewport.height = static_cast<float>(extent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(frame.commandBuffer, 0, 1, &viewport);

		VkRect2D scissor = { {0, 0}, extent };
		vkCmdSetScissor(frame.commandBuffer, 0, 1, &scissor);

		const VkBuffer vertexBuffers[] = { storage.globalBuffers.vertexBuffer };
		constexpr VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(frame.commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(frame.commandBuffer, storage.globalBuffers.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		// Bind global descriptor (camera view/proj)
		vkCmdBindDescriptorSets(
			frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&storage.globalDescriptorSets[frame.currentFrame], 0, nullptr);

		for (const MeshInstance& meshInstance : scene.meshInstances)
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

	void DepthPrePass::cleanup(const VulkanStorage& storage)
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

	void DepthPrePass::onSwapchainRecreate(VulkanStorage& storage, const VulkanConfigurator& config)
	{
		cleanup(storage);
		init(storage, config);
	}

}
