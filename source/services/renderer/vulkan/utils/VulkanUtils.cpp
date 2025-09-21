#include "VulkanUtils.h"

#include <mutex>

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
        std::lock_guard lock(vulkanStorage.graphicsQueueMutex);
        return vkQueueSubmit(vulkanStorage.graphicsQueue, 1, submitInfo, fence);
    }

    VkResult threadSafePresent(VulkanStorage& vulkanStorage, const VkPresentInfoKHR* presentInfo)
    {
        std::lock_guard lock(vulkanStorage.graphicsQueueMutex);
        return vkQueuePresentKHR(vulkanStorage.graphicsQueue, presentInfo);
    }
}
