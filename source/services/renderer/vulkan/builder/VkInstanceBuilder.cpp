#include "VkInstanceBuilder.h"

#include "engine/EngineCore.h"

namespace parus::vulkan
{
    
    void VkInstanceBuilder::build(VulkanStorage& storage) const
    {
        const VkApplicationInfo appInfo = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = applicationName.c_str(),
            .applicationVersion = VK_MAKE_API_VERSION(1, versionMajor, versionMinor, versionPatch),
            .pEngineName = applicationName.c_str(),
            .engineVersion = VK_MAKE_API_VERSION(1, versionMajor, versionMinor, versionPatch),
            .apiVersion = VK_API_VERSION_1_0,
        };

        const VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = debugCallback,
            .pUserData = nullptr,
        };

        const VkInstanceCreateInfo createInfo = {
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = &debugCreateInfo,
            .flags = 0,
            .pApplicationInfo = &appInfo,
            .enabledLayerCount = static_cast<uint32_t>(validationLayers.size()),
            .ppEnabledLayerNames = validationLayers.data(),
            .enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size()),
            .ppEnabledExtensionNames = requiredExtensions.data(),
        };

        ASSERT(vkCreateInstance(&createInfo, nullptr, &storage.instance) == VK_SUCCESS, "failed to create instance");
    }

    VkInstanceBuilder& VkInstanceBuilder::setApplicationName(const std::string& newApplicationName)
    {
        applicationName = newApplicationName;
        return *this;
    }

    VkInstanceBuilder& VkInstanceBuilder::setVersion(const int newVersionMajor,
        const int newVersionMinor,
        const int newVersionPatch)
    {
        versionMajor = newVersionMajor;
        versionMinor = newVersionMinor;
        versionPatch = newVersionPatch;
        return *this;
    }

    VkInstanceBuilder& VkInstanceBuilder::setRequiredExtensions(const std::vector<const char*>& newRequiredExtensions)
    {
        requiredExtensions = newRequiredExtensions;
        return *this;
    }

    VkInstanceBuilder& VkInstanceBuilder::setValidationLayers(const std::vector<const char*>& newValidationLayers)
    {
        validationLayers = newValidationLayers;
        return *this;
    }

    VkInstanceBuilder& VkInstanceBuilder::setDebugCallback(const PFN_vkDebugUtilsMessengerCallbackEXT& newDebugCallback)
    {
        debugCallback = newDebugCallback;
        return *this;
    }
}
