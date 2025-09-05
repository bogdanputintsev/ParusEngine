#include "VkDebugUtilsBuilder.h"

#include "engine/EngineCore.h"

namespace parus::vulkan
{
    
    void VkDebugUtilsBuilder::build(VulkanStorage& storage) const
    {
        const VkDebugUtilsMessengerCreateInfoEXT createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback,
            .pUserData = nullptr,
        };
        
        ASSERT(createDebugUtilsMessengerExt(&createInfo, nullptr, storage) == VK_SUCCESS, "failed to set up debug messenger.");

        // Setup debug object name extension
        storage.vkSetDebugUtilsObjectNameEXT
            = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(storage.instance, "vkSetDebugUtilsObjectNameEXT"));

        ASSERT(storage.vkSetDebugUtilsObjectNameEXT, "Failed to load vkSetDebugUtilsObjectNameEXT");
    }

    VkResult VkDebugUtilsBuilder::createDebugUtilsMessengerExt(
        const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
        const VkAllocationCallbacks* pAllocator,
        VulkanStorage& storage)
    {
        const auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(storage.instance, "vkCreateDebugUtilsMessengerEXT"));
        if (func != nullptr)
        {
            return func(storage.instance, pCreateInfo, pAllocator, &storage.debugMessenger);
        }

        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
    
    VkDebugUtilsBuilder& VkDebugUtilsBuilder::setDebugCallback(const PFN_vkDebugUtilsMessengerCallbackEXT& newDebugCallback)
    {
        debugCallback = newDebugCallback;
        return *this;
    }
}
