#pragma once
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    
    class VkInstanceBuilder
    {
    public:
        void build(VulkanStorage& storage) const;

        VkInstanceBuilder& setApplicationName(const std::string& newApplicationName);
        VkInstanceBuilder& setVersion(const int newVersionMajor, const int newVersionMinor, const int newVersionPatch);
        VkInstanceBuilder& setRequiredExtensions(const std::vector<const char*>& newRequiredExtensions);
        VkInstanceBuilder& setValidationLayers(const std::vector<const char*>& newValidationLayers);
        VkInstanceBuilder& setDebugCallback(const PFN_vkDebugUtilsMessengerCallbackEXT& newDebugCallback);
    private:
        std::string applicationName;
        int versionMajor = 0;
        int versionMinor = 0;
        int versionPatch = 0;
        std::vector<const char*> requiredExtensions;
        std::vector<const char*> validationLayers;
        PFN_vkDebugUtilsMessengerCallbackEXT debugCallback{};
        
    };
    
}
