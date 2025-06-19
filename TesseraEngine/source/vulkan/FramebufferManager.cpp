#include "FramebufferManager.h"

#include <stdexcept>

#include "SwapChainManager.h"

namespace tessera::vulkan
{

	void FramebufferManager::init(const std::vector<VkImageView>& swapChainImageViews, const std::shared_ptr<const VkDevice>& device, const std::shared_ptr<VkRenderPass>& renderPass, const SwapChainImageDetails& swapChainImageDetails)
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); ++i)
        {
	        const VkImageView attachments[] = {
		        swapChainImageViews[i]
	        };

            const auto& [width, height] = swapChainImageDetails.swapChainExtent;

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = *renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = width;
            framebufferInfo.height = height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(*device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) 
            {
                throw std::runtime_error("VulkanFramebufferManager: failed to create framebuffer.");
            }
        }
	}

	void FramebufferManager::clean(const std::shared_ptr<const VkDevice>& device) const
	{
        for (const auto& framebuffer : swapChainFramebuffers) 
        {
            vkDestroyFramebuffer(*device, framebuffer, nullptr);
        }
	}
}
