#pragma once
#include <vulkan/vulkan_core.h>

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{

    class VkDebugUtilsBuilder
    {
    public:
        void build(VulkanStorage& storage) const;

        VkDebugUtilsBuilder& setDebugCallback(const PFN_vkDebugUtilsMessengerCallbackEXT& newDebugCallback);

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
            VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
            void* pUserData);

        static void destroy(VulkanStorage& storage);

    private:
        static VkResult createDebugUtilsMessengerExt(const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo,
                                                     const VkAllocationCallbacks* pAllocator, VulkanStorage& storage);

        PFN_vkDebugUtilsMessengerCallbackEXT callbackFn{};
    };

}

