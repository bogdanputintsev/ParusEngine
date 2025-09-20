#include "VkSurfaceBuilder.h"

#include "engine/EngineCore.h"
#include "services/platform/Platform.h"

#include <vulkan/vulkan_win32.h>

namespace parus::vulkan
{

    void VkSurfaceBuilder::build(VulkanStorage& vulkanStorage)
    {
        const PlatformStorage platformStorage = Services::get<Platform>()->getPlatformStorage();

#ifdef WITH_WINDOWS_PLATFORM
        const VkWin32SurfaceCreateInfoKHR createInfo = {
            .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
            .pNext = nullptr,
            .flags = 0,
            .hinstance = platformStorage.hinstance,
            .hwnd = platformStorage.hwnd,
        };
        
        ASSERT(vkCreateWin32SurfaceKHR(vulkanStorage.instance, &createInfo, nullptr, &vulkanStorage.surface) == VK_SUCCESS,
               "Vulkan surface creation failed");
#endif
        
    }
}
