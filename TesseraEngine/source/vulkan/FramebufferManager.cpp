#include "FramebufferManager.h"

#include <stdexcept>

#include "GraphicsPipelineManager.h"
#include "ImageViewManager.h"
#include "SwapChainManager.h"
#include "utils/interfaces/ServiceLocator.h"

namespace tessera::vulkan
{

	void FramebufferManager::init()
	{
        const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();
        const auto& swapChainImageViews = ServiceLocator::getService<ImageViewManager>()->getSwapChainImageViews();
        const auto& swapChainImageDetails = ServiceLocator::getService<SwapChainManager>()->getSwapChainImageDetails();
        const auto& renderPass = ServiceLocator::getService<GraphicsPipelineManager>()->getRenderPath();

		swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); ++i)
        {
	        const VkImageView attachments[] = {
		        swapChainImageViews[i]
	        };

            const auto& [width, height] = swapChainImageDetails.swapChainExtent;

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) 
            {
                throw std::runtime_error("VulkanFramebufferManager: failed to create framebuffer.");
            }
        }
	}

	void FramebufferManager::clean()
	{
        const auto& device = ServiceLocator::getService<DeviceManager>()->getLogicalDevice();

        for (const auto& framebuffer : swapChainFramebuffers) 
        {
            vkDestroyFramebuffer(device, framebuffer, nullptr);
        }
	}
}
