#include "VkDebugUtilsBuilder.h"

#include "engine/EngineCore.h"

namespace parus::vulkan
{

    namespace
    {
        LogType getLogType(const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity)
        {
            switch (messageSeverity)
            {
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:   return LogType::LOG_TYPE_DEBUG;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:      return LogType::LOG_TYPE_INFO;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:   return LogType::LOG_TYPE_WARNING;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:     return LogType::LOG_TYPE_ERROR;
            case VK_DEBUG_UTILS_MESSAGE_SEVERITY_FLAG_BITS_MAX_ENUM_EXT: return LogType::LOG_TYPE_INFO;
            }
            return LogType::LOG_TYPE_INFO;
        }
    }

    void VkDebugUtilsBuilder::build(VulkanStorage& storage) const
    {
#ifdef IN_DEBUG_MODE
        const VkDebugUtilsMessengerCreateInfoEXT createInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = callbackFn,
            .pUserData = nullptr,
        };

        ASSERT(createDebugUtilsMessengerExt(&createInfo, nullptr, storage) == VK_SUCCESS, "failed to set up debug messenger.");

        // Setup debug object name extension
        storage.vkSetDebugUtilsObjectNameEXT
            = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(storage.instance, "vkSetDebugUtilsObjectNameEXT"));

        ASSERT(storage.vkSetDebugUtilsObjectNameEXT, "Failed to load vkSetDebugUtilsObjectNameEXT");
#endif
    }

    VkBool32 VKAPI_ATTR VKAPI_CALL VkDebugUtilsBuilder::debugCallback(
        const VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        [[maybe_unused]] const VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        [[maybe_unused]] void* pUserData)
    {
        LOG(getLogType(messageSeverity), pCallbackData->pMessage);
        return VK_FALSE;
    }

    void VkDebugUtilsBuilder::destroy(VulkanStorage& storage)
    {
#ifdef IN_DEBUG_MODE
        const auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
            vkGetInstanceProcAddr(storage.instance, "vkDestroyDebugUtilsMessengerEXT"));
        if (func != nullptr)
        {
            func(storage.instance, storage.debugMessenger, nullptr);
        }
#endif
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
        callbackFn = newDebugCallback;
        return *this;
    }
}
