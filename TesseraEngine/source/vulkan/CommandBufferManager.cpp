#include "CommandBufferManager.h"

#include <stdexcept>

#include "DeviceManager.h"
#include "FramebufferManager.h"
#include "GraphicsPipelineManager.h"
#include "QueueManager.h"
#include "SurfaceManager.h"
#include "utils/interfaces/ServiceLocator.h"

namespace tessera::vulkan
{

	void CommandBufferManager::init()
	{
		initCommandPool();
		initCommandBuffer();
	}

	void CommandBufferManager::initCommandPool()
	{
		const auto& deviceManager = ServiceLocator::getService<DeviceManager>();
		const auto& physicalDevice = deviceManager->getPhysicalDevice();
		const auto& device = deviceManager->getLogicalDevice();
		const auto& surface = ServiceLocator::getService<SurfaceManager>()->getSurface();

		const auto [graphicsFamily, presentFamily] = findQueueFamilies(*physicalDevice, surface);
		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = graphicsFamily.value();

		VkCommandPool commandPoolInstance;
		if (vkCreateCommandPool(*device, &poolInfo, nullptr, &commandPoolInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to create command pool.");
		}

		commandPool = std::make_shared<VkCommandPool>(commandPoolInstance);
	}

	void CommandBufferManager::initCommandBuffer()
	{
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = *commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBufferInstance;
		if (vkAllocateCommandBuffers(*device, &allocInfo, &commandBufferInstance) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to allocate command buffers.");
		}

		commandBuffer = std::make_shared<VkCommandBuffer>(commandBufferInstance);
	}

	void CommandBufferManager::recordCommandBuffer(const VkCommandBuffer commandBufferToRecord, const uint32_t imageIndex)
	{
		const auto& swapChainFramebuffers = ServiceLocator::getService<FramebufferManager>()->getSwapChainFramebuffers();
		const auto& [swapChainImageFormat, swapChainExtent, swapChainImages]
			= ServiceLocator::getService<SwapChainManager>()->getSwapChainImageDetails();
		const auto& graphicsPipelineManager = ServiceLocator::getService<GraphicsPipelineManager>();
		const auto& graphicsPipeline = graphicsPipelineManager->getGraphicsPipeline();
		const auto& renderPass = graphicsPipelineManager->getRenderPath();

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBufferToRecord, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to begin recording command buffer.");
		}

		// Start the rendering pass
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = *renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;
		renderPassInfo.clearValueCount = 1;
		constexpr VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.pClearValues = &clearColor;

		// Draw.
		vkCmdBeginRenderPass(commandBufferToRecord, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
			// TODO: Maybe RAII initialization?
			// Bind the graphics pipeline.
			vkCmdBindPipeline(commandBufferToRecord, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
			VkViewport viewport;
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(swapChainExtent.width);
			viewport.height = static_cast<float>(swapChainExtent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;
			vkCmdSetViewport(commandBufferToRecord, 0, 1, &viewport);

			VkRect2D scissor;
			scissor.offset = { 0, 0 };
			scissor.extent = swapChainExtent;
			vkCmdSetScissor(commandBufferToRecord, 0, 1, &scissor);

			vkCmdDraw(commandBufferToRecord, 3, 1, 0, 0);
		vkCmdEndRenderPass(commandBufferToRecord);

		if (vkEndCommandBuffer(commandBufferToRecord) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to record command buffer");
		}
	}

	void CommandBufferManager::resetCommandBuffer() const
	{
		vkResetCommandBuffer(*commandBuffer, 0);
	}

	void CommandBufferManager::clean()
	{
		const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

		vkDestroyCommandPool(*device, *commandPool, nullptr);
	}

}
