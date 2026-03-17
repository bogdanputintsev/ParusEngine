#include "VkCommandBufferBuilder.h"

#include "services/renderer/vulkan/utils/VulkanUtils.h"

namespace parus::vulkan
{
    std::vector<VkCommandBuffer> VkCommandBufferBuilder::build(const std::string& name, const VulkanStorage& storage) const
    {
        ASSERT(commandPool != VK_NULL_HANDLE, "Command pool must be set before building command buffers.");
        ASSERT(count > 0, "Command buffer count must be greater than zero.");

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = count;

        std::vector<VkCommandBuffer> commandBuffers(count);
        ASSERT(vkAllocateCommandBuffers(storage.logicalDevice, &allocInfo, commandBuffers.data()) == VK_SUCCESS,
            "Failed to allocate command buffers " + name);

        for (uint32_t i = 0; i < count; ++i)
        {
            utils::setDebugName(storage, commandBuffers[i], VK_OBJECT_TYPE_COMMAND_BUFFER,
                (name + " [" + std::to_string(i) + "]").c_str());
        }

        return commandBuffers;
    }

    VkCommandBufferBuilder& VkCommandBufferBuilder::setCommandPool(const VkCommandPool newCommandPool)
    {
        commandPool = newCommandPool;
        return *this;
    }

    VkCommandBufferBuilder& VkCommandBufferBuilder::setCount(const uint32_t newCount)
    {
        count = newCount;
        return *this;
    }
}