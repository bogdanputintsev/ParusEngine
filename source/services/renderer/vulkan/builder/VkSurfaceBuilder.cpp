#include "VkSurfaceBuilder.h"

#include <vulkan/vulkan_win32.h>

#include "engine/EngineCore.h"

namespace parus::vulkan
{

    void VkSurfaceBuilder::build(const HINSTANCE& hinstance, const HWND& hwnd, VulkanStorage& storage)
    {
        // const VkWin32SurfaceCreateInfoKHR createInfo = {
        //     .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        //     .pNext = nullptr,
        //     .flags = 0,
        //     .hinstance = hinstance,
        //     .hwnd = hwnd,
        // };
        //
        //
        // ASSERT(vkCreateWin32SurfaceKHR(storage.instance, &createInfo, nullptr, &storage.surface) == VK_SUCCESS,
        //        "Vulkan surface creation failed");
    }
}
