#include "VulkanUtils.h"

#include <mutex>

#include "services/renderer/vulkan/builder/VkCommandPoolFactory.h"
#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan::utils
{
    void setDebugName(const VulkanStorage& vulkanStorage, const void* objectHandle, const VkObjectType objectType, const char* name)
    {
        if (vulkanStorage.vkSetDebugUtilsObjectNameEXT)
        {
            VkDebugUtilsObjectNameInfoEXT nameInfo{};
            nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
            nameInfo.objectType = objectType;
            nameInfo.objectHandle = reinterpret_cast<uint64_t>(objectHandle);
            nameInfo.pObjectName = name;

            vulkanStorage.vkSetDebugUtilsObjectNameEXT(vulkanStorage.logicalDevice, &nameInfo);
        }
        else
        {
            LOG_WARNING("VkDebugUtilsObjectNameInfoEXT is not properly set.");
        }
    }

    QueueFamilyIndices findQueueFamilies(
        const VkPhysicalDevice& physicalDevice,
        const VkSurfaceKHR& surface)
    {
        QueueFamilyIndices familyIndices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                familyIndices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
            if (presentSupport)
            {
                familyIndices.presentFamily = i;
            }

            if (familyIndices.isComplete())
            {
                break;
            }
        }

        return familyIndices;
    }

    SwapChainSupportDetails querySwapChainSupport(const VkPhysicalDevice& device, const VkSurfaceKHR& surface)
    {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0)
        {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0)
        {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkResult threadSafeQueueSubmit(VulkanStorage& vulkanStorage, const VkSubmitInfo* submitInfo, VkFence fence)
    {
        std::scoped_lock lock(vulkanStorage.graphicsQueueMutex);
        return vkQueueSubmit(vulkanStorage.graphicsQueue, 1, submitInfo, fence);
    }

    VkResult threadSafePresent(VulkanStorage& vulkanStorage, const VkPresentInfoKHR* presentInfo)
    {
        std::scoped_lock lock(vulkanStorage.graphicsQueueMutex);
        return vkQueuePresentKHR(vulkanStorage.graphicsQueue, presentInfo);
    }

    VkFormat findSupportedFormat(const VulkanStorage& vulkanStorage, const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
    {
        std::optional<VkFormat> supportedFormat;
        
        for (VkFormat format : candidates)
        {
            VkFormatProperties props;
            vkGetPhysicalDeviceFormatProperties(vulkanStorage.physicalDevice, format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
            {
                supportedFormat = format;
            }
            else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
            {
                supportedFormat = format;
            }
        }

        ASSERT(supportedFormat.has_value(), "failed to find supported format.");

        return supportedFormat.value();
    }

    VkFormat findDepthFormat(const VulkanStorage& vulkanStorage)
    {
        return findSupportedFormat(
            vulkanStorage,
            { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
        );
    }

    VkCommandBuffer beginSingleTimeCommands(const VulkanStorage& storage, const VkCommandPool commandPool)
    {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = commandPool;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(storage.logicalDevice, &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VulkanStorage& storage, const VkCommandPool commandPool, const VkCommandBuffer commandBuffer)
    {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        threadSafeQueueSubmit(storage, &submitInfo, nullptr);

        {
            std::scoped_lock lock(storage.graphicsQueueMutex);
            vkQueueWaitIdle(storage.graphicsQueue);
        }

        vkFreeCommandBuffers(storage.logicalDevice, commandPool, 1, &commandBuffer);
    }

    uint32_t findMemoryType(const VulkanStorage& vulkanStorage, const uint32_t typeFilter, const VkMemoryPropertyFlags properties)
    {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(vulkanStorage.physicalDevice, &memProperties);

        std::optional<uint32_t> memoryIndex;

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
        {
            if (typeFilter & 1 << i && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
            {
                memoryIndex = i;
                break;
            }
        }

        ASSERT(memoryIndex.has_value(), "failed to find suitable memory type.");
        return memoryIndex.value();
    }

    VkCommandPool createCommandPool(const VulkanStorage& storage)
    {
        const std::thread::id threadId = std::this_thread::get_id();
        return VkCommandPoolFactory().build("Command Pool [thread " + std::to_string(std::hash<std::thread::id>{}(threadId)) + "]", storage);
    }

    VkCommandPool getCommandPool(VulkanStorage& storage)
    {
        const std::thread::id threadId = std::this_thread::get_id();
        if (!storage.threadCommandPools.contains(threadId))
        {
            storage.threadCommandPools[threadId] = createCommandPool(storage);
        }

        return storage.threadCommandPools[threadId];
    }
}
