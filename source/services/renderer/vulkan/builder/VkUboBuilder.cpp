#include "VkUboBuilder.h"

#include "VkBufferBuilder.h"

namespace parus::vulkan
{
    UboBuffer VkUboBuilder::build(const std::string& name, const VulkanStorage& storage) const
    {
        ASSERT(size > 0, "UBO size must not be empty");

        UboBuffer result;
        for (int i = 0; i < UboBuffer::MAX_FRAMES_IN_FLIGHT; ++i)
        {
            std::tie(result.frameBuffers[i], result.memory[i]) = VkBufferBuilder(name + " [" + std::to_string(i) + "]")
                .setSize(size)
                .setUsage(VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
                .setMemoryProperties(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
                .build(storage);

            ASSERT(result.frameBuffers[i] != VK_NULL_HANDLE, name + " buffer must be valid");

            vkMapMemory(storage.logicalDevice, result.memory[i], 0, size, 0, &result.mapped[i]);
        }

        return result;
    }

    VkUboBuilder& VkUboBuilder::setSize(const VkDeviceSize newSize)
    {
        size = newSize;
        return *this;
    }
}