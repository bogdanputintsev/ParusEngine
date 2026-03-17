#pragma once

#include <string>
#include <vulkan/vulkan_core.h>
#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkFramebufferBuilder final
    {
    public:
        [[nodiscard]] VkFramebuffer build(const std::string& name, const VulkanStorage& storage) const;

        VkFramebufferBuilder& setColorImageView(VkImageView newColorImageView);
        VkFramebufferBuilder& setDepthImageView(VkImageView newDepthImageView);
        VkFramebufferBuilder& setSwapChainImageView(VkImageView newSwapChainImageView);

    private:
        VkImageView colorImageView = VK_NULL_HANDLE;
        VkImageView depthImageView = VK_NULL_HANDLE;
        VkImageView swapChainImageView = VK_NULL_HANDLE;
    };
}