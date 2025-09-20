#pragma once

#include <vector>

#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkSwapChainBuilder
    {
    public:
        static void build(VulkanStorage& storage);
    private:
        static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    };
}
