#include "VkCommandPoolFactory.h"

#include "services/renderer/vulkan/utils/VulkanUtils.h"

namespace parus::vulkan
{
    VkCommandPool VkCommandPoolFactory::build(const std::string& name, const VulkanStorage& storage) const
    {
        const auto [graphicsFamily, presentFamily] = utils::findQueueFamilies(storage.physicalDevice, storage.surface);
        ASSERT(graphicsFamily.has_value(), "Graphics family is incomplete.");

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = graphicsFamily.value();

        VkCommandPool commandPool;
        ASSERT(vkCreateCommandPool(storage.logicalDevice, &poolInfo, nullptr, &commandPool) == VK_SUCCESS,
            "Failed to create command pool " + name);
        utils::setDebugName(storage, commandPool, VK_OBJECT_TYPE_COMMAND_POOL, name.c_str());

        return commandPool;
    }
}