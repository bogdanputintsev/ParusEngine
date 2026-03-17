#include "VkFramebufferBuilder.h"

#include <array>

namespace parus::vulkan
{
    VkFramebuffer VkFramebufferBuilder::build(const std::string& name, const VulkanStorage& storage) const
    {
        const std::array attachments = { colorImageView, depthImageView, swapChainImageView };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = storage.renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = storage.swapChainDetails.swapChainExtent.width;
        framebufferInfo.height = storage.swapChainDetails.swapChainExtent.height;
        framebufferInfo.layers = 1;

        VkFramebuffer framebuffer;
        ASSERT(vkCreateFramebuffer(storage.logicalDevice, &framebufferInfo, nullptr, &framebuffer) == VK_SUCCESS,
            "failed to create framebuffer.");
        utils::setDebugName(storage, framebuffer, VK_OBJECT_TYPE_FRAMEBUFFER, name.c_str());

        return framebuffer;
    }

    VkFramebufferBuilder& VkFramebufferBuilder::setColorImageView(const VkImageView newColorImageView)
    {
        colorImageView = newColorImageView;
        return *this;
    }

    VkFramebufferBuilder& VkFramebufferBuilder::setDepthImageView(const VkImageView newDepthImageView)
    {
        depthImageView = newDepthImageView;
        return *this;
    }

    VkFramebufferBuilder& VkFramebufferBuilder::setSwapChainImageView(const VkImageView newSwapChainImageView)
    {
        swapChainImageView = newSwapChainImageView;
        return *this;
    }
}