#include "VkBufferBuilder.h"

#include "services/renderer/vulkan/utils/VulkanUtils.h"

namespace parus::vulkan
{
    VkBufferBuilder::VkBufferBuilder(std::string name)
        : debugName(std::move(name))
    {
    }

    std::pair<VkBuffer, VkDeviceMemory> VkBufferBuilder::build(const VulkanStorage& storage) const
    {
        ASSERT(size > 0, "Buffer size must not be empty");

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer buffer;
        ASSERT(vkCreateBuffer(storage.logicalDevice, &bufferInfo, nullptr, &buffer) == VK_SUCCESS,
            "Failed to create buffer " + debugName);
        ASSERT(buffer != VK_NULL_HANDLE, "Buffer must be valid.");
        utils::setDebugName(storage, buffer, VK_OBJECT_TYPE_BUFFER, debugName.c_str());

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(storage.logicalDevice, buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = utils::findMemoryType(storage, memRequirements.memoryTypeBits, memoryProperties);

        VkDeviceMemory memory;
        ASSERT(vkAllocateMemory(storage.logicalDevice, &allocInfo, nullptr, &memory) == VK_SUCCESS,
            "Failed to allocate buffer memory for " + debugName);
        ASSERT(vkBindBufferMemory(storage.logicalDevice, buffer, memory, 0) == VK_SUCCESS,
            "Failed to bind buffer memory for " + debugName);

        return { buffer, memory };
    }

    VkBufferBuilder& VkBufferBuilder::setSize(const VkDeviceSize newSize)
    {
        size = newSize;
        return *this;
    }

    VkBufferBuilder& VkBufferBuilder::setUsage(const VkBufferUsageFlags newUsage)
    {
        usage = newUsage;
        return *this;
    }

    VkBufferBuilder& VkBufferBuilder::setMemoryProperties(const VkMemoryPropertyFlags newMemoryProperties)
    {
        memoryProperties = newMemoryProperties;
        return *this;
    }
}