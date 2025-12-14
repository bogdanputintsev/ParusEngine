#include "VkDeviceMemoryBuilder.h"


namespace parus::vulkan
{
    VkDeviceMemoryBuilder::VkDeviceMemoryBuilder(std::string name)
        : debugName(std::move(name))
    {
    }

    VkDeviceMemory VkDeviceMemoryBuilder::build(const VulkanStorage& storage) const
    {
        ASSERT(image, "Image must be set for VkDeviceMemoryBuilder");
        
        VkDeviceMemory imageMemory;
        
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(storage.logicalDevice, image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = utils::findMemoryType(storage, memRequirements.memoryTypeBits, propertyFlags);

        ASSERT(vkAllocateMemory(storage.logicalDevice, &allocInfo, nullptr, &imageMemory) == VK_SUCCESS, "failed to allocate image memory.");
        utils::setDebugName(storage, imageMemory, VK_OBJECT_TYPE_DEVICE_MEMORY, debugName.c_str());

        vkBindImageMemory(storage.logicalDevice, image, imageMemory, 0);

        return imageMemory;
    }

    VkDeviceMemoryBuilder& VkDeviceMemoryBuilder::setImage(const VkImage& newImage)
    {
        image = newImage;
        return *this;
    }

    VkDeviceMemoryBuilder& VkDeviceMemoryBuilder::setPropertyFlags(const VkMemoryPropertyFlags newPropertyFlags)
    {
        propertyFlags = newPropertyFlags;
        return *this;
    }
}
