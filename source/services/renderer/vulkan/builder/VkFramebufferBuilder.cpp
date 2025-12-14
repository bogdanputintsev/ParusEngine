#include "VkFramebufferBuilder.h"

#include "VkImageViewBuilder.h"

namespace parus::vulkan
{
    VkFramebufferBuilder& VkFramebufferBuilder::addColorImageView(VulkanStorage& storage)
    {
        // utils::createImage(
        //     storage.swapChainDetails.swapChainExtent.width,
        //     storage.swapChainDetails.swapChainExtent.height,
        //     1,
        //     storage.msaaSamples,
        //     storage.swapChainDetails.swapChainImageFormat,
        //     VK_IMAGE_TILING_OPTIMAL,
        //     VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        //     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        //     colorImage,
        //     colorImageMemory);
        //
        // colorImageView = VkImageViewBuilder()
        //     .setImage(colorImage)
        //     .setFormat(colorFormat)
        //     .setAspectMask(VK_IMAGE_ASPECT_COLOR_BIT)
        //     .setLevelCount(1)
        //     .build("Color Image View", storage);
        return *this;
    }
}
