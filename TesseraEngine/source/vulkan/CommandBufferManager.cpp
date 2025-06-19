#include "CommandBufferManager.h"

#include <stdexcept>

#include "QueueFamiliesManager.h"

namespace tessera::vulkan
{

	void CommandBufferManager::init(const std::shared_ptr<const VkDevice>& device, const std::shared_ptr<const VkPhysicalDevice>& physicalDevice, const std::shared_ptr<const VkSurfaceKHR>& surface, const std::shared_ptr<VkRenderPass>& renderPass)
	{
		initCommandPool(device, physicalDevice, surface);
		initCommandBuffer(device);
	}

	void CommandBufferManager::initCommandPool(const std::shared_ptr<const VkDevice>& device, const std::shared_ptr<const VkPhysicalDevice>& physicalDevice, const std::shared_ptr<const VkSurfaceKHR>& surface)
	{
		const auto [graphicsFamily, presentFamily] = QueueFamiliesManager::findQueueFamilies(*physicalDevice, surface);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = graphicsFamily.value();

		if (vkCreateCommandPool(*device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to create command pool.");
		}
	}

	void CommandBufferManager::initCommandBuffer(const std::shared_ptr<const VkDevice>& device)
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = 1;

		if (vkAllocateCommandBuffers(*device, &allocInfo, &commandBuffer) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to allocate command buffers.");
		}
	}

	void CommandBufferManager::recordCommandBuffer(const VkCommandBuffer commandBufferToRecord, uint32_t imageIndex, const std::shared_ptr<VkRenderPass>& renderPass)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0;
		beginInfo.pInheritanceInfo = nullptr;

		if (vkBeginCommandBuffer(commandBufferToRecord, &beginInfo) != VK_SUCCESS)
		{
			throw std::runtime_error("CommandPoolManager: failed to begin recording command buffer.");
		}

		//// Start the rendering pass
		//VkRenderPassBeginInfo renderPassInfo{};
		//renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		//renderPassInfo.renderPass = *renderPass;
		//renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
	}

	void CommandBufferManager::clean(const std::shared_ptr<const VkDevice>& device) const
	{
		vkDestroyCommandPool(*device, commandPool, nullptr);
	}

}
