#include "SSAOPass.h"

#include <array>

#include "DepthPrePass.h"
#include "services/renderer/vulkan/storage/VulkanStorage.h"
#include "services/renderer/vulkan/VulkanConfigurator.h"
#include "services/renderer/vulkan/VulkanDescriptorManager.h"
#include "services/renderer/vulkan/builder/VkImageBuilder.h"
#include "services/renderer/vulkan/builder/VkImageViewBuilder.h"
#include "services/renderer/vulkan/builder/VkDeviceMemoryBuilder.h"
#include "services/renderer/vulkan/builder/VkDescriptorSetLayoutBuilder.h"
#include "services/renderer/vulkan/builder/VkPipelineBuilder.h"
#include "services/renderer/vulkan/utils/VulkanUtils.h"


namespace parus::vulkan
{

	void SSAOPass::init(VulkanStorage& storage, const VulkanConfigurator& config)
	{
		const uint32_t screenWidth = storage.swapChainDetails.swapChainExtent.width;
		const uint32_t screenHeight = storage.swapChainDetails.swapChainExtent.height;

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

			VkRenderPassCreateInfo renderPassCreateInfo{};
			renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderPassCreateInfo.attachmentCount = 1;
			renderPassCreateInfo.pAttachments = &colorAttachment;
			renderPassCreateInfo.subpassCount = 1;
			renderPassCreateInfo.pSubpasses = &subpass;
			renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
			renderPassCreateInfo.pDependencies = dependencies.data();

			ASSERT(vkCreateRenderPass(storage.logicalDevice, &renderPassCreateInfo, nullptr, &renderPass) == VK_SUCCESS,
				"Failed to create SSAO render pass.");
			utils::setDebugName(storage, renderPass, VK_OBJECT_TYPE_RENDER_PASS, "SSAO Render Pass");
		}

		// SSAO image
		image = VkImageBuilder("SSAO Image")
			.setWidth(screenWidth)
			.setHeight(screenHeight)
			.setFormat(VK_FORMAT_R16_SFLOAT)
			.setTiling(VK_IMAGE_TILING_OPTIMAL)
			.setUsage(VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT)
			.build(storage);

		imageMemory = VkDeviceMemoryBuilder("SSAO Image Memory")
			.setImage(image)
			.setPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
			.build(storage);

		imageView = VkImageViewBuilder()
			.setImage(image)
			.setViewType(VK_IMAGE_VIEW_TYPE_2D)
			.setFormat(VK_FORMAT_R16_SFLOAT)
			.setAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
			.build("SSAO Image View", storage);

		// SSAO framebuffer
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
				"Failed to create SSAO framebuffer.");
			utils::setDebugName(storage, framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, "SSAO Framebuffer");
		}
	}

	void SSAOPass::initPipeline(VulkanStorage& storage, const VulkanDescriptorManager& descriptorManager, const DepthPrePass& depthPrePass)
	{
		using DescriptorType = VulkanDescriptorManager::DescriptorType;

		// SSAO descriptor set layout
		descriptorSetLayout = VkDescriptorSetLayoutBuilder("SSAO Descriptor Set Layout")
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
			allocInfo.pSetLayouts = &descriptorSetLayout;

			ASSERT(vkAllocateDescriptorSets(storage.logicalDevice, &allocInfo, &descriptorSet) == VK_SUCCESS,
				"Failed to allocate SSAO descriptor set.");

			const VkDescriptorImageInfo depthImageInfo =
			{
				.sampler = depthPrePass.getSampler(),
				.imageView = depthPrePass.getImageView(),
				.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
			};

			const VkWriteDescriptorSet write =
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.pNext = nullptr,
				.dstSet = descriptorSet,
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
			.setRenderPass(renderPass)
			.useLayouts({
				descriptorManager.getLayout(DescriptorType::GLOBAL),
				descriptorSetLayout
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
			.build(storage, pipelineLayout, pipeline);
	}

	void SSAOPass::record(const FrameContext& frame, const VulkanStorage& storage, const SceneData& scene) const
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
		clearValue.color = {{ 1.0f, 1.0f, 1.0f, 1.0f }};
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

		// Bind global UBO (set 0) - contains proj matrix for SSAO reconstruction
		vkCmdBindDescriptorSets(
			frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			0,
			1,
			&storage.globalDescriptorSets[frame.currentFrame], 0, nullptr);

		// Bind SSAO descriptor (set 1) - contains depth pre-pass texture
		vkCmdBindDescriptorSets(
			frame.commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			pipelineLayout,
			1,
			1,
			&descriptorSet, 0, nullptr);

		// Fullscreen triangle - 3 vertices, no vertex buffer
		vkCmdDraw(frame.commandBuffer, 3, 1, 0, 0);

		vkCmdEndRenderPass(frame.commandBuffer);
	}

	void SSAOPass::cleanup(const VulkanStorage& storage)
	{
		if (descriptorSetLayout != VK_NULL_HANDLE)
		{
			vkDestroyDescriptorSetLayout(storage.logicalDevice, descriptorSetLayout, nullptr);
			descriptorSetLayout = VK_NULL_HANDLE;
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

	void SSAOPass::onSwapchainRecreate(VulkanStorage& storage, const VulkanConfigurator& config)
	{
		cleanup(storage);
		init(storage, config);
	}

}
