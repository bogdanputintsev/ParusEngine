#pragma once

#include "services/platform/Platform.h"
#include "services/renderer/vulkan/storage/VulkanStorage.h"

namespace parus::vulkan
{
    class VkSurfaceBuilder
    {
    public:
        static void build(const HINSTANCE& hinstance, const HWND& hwnd, VulkanStorage& storage);
    };
}   